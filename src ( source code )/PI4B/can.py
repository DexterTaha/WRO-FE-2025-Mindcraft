from picamera2 import Picamera2
import cv2
import numpy as np

def initialize_camera():
    """Initialize and configure the Picamera2."""
    picam2 = Picamera2(camera_num=0)
    config = picam2.create_preview_configuration()
    picam2.configure(config)
    picam2.start()
    return picam2

def capture_frame(picam2, flip=False, resize_shape=(640, 480)):
    """Capture a frame from the camera, resize, and optionally flip it vertically."""
    frame = picam2.capture_array()
    frame = cv2.resize(frame, resize_shape)
    if flip:
        frame = cv2.flip(frame, 0)  # Flip vertically
        frame = cv2.flip(frame, 1)  # Flip horizontally
    return frame

def convert_to_hsv(frame):
    """Convert the frame from BGR to HSV color space."""
    return cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

def create_color_masks(hsv_frame, lower_red, upper_red, lower_green, upper_green):
    """Create color masks for red and green in the HSV frame."""
    mask_red = cv2.inRange(hsv_frame, lower_red, upper_red)
    mask_green = cv2.inRange(hsv_frame, lower_green, upper_green)
    return mask_red, mask_green

def find_and_store_contours(mask, color_label, rectangle_color, label_color):
    """Find and store contours along with their areas for both close and far objects."""
    contours_info = []
    cnts, _ = cv2.findContours(mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_NONE)

    for c in cnts:
        area = cv2.contourArea(c)
        if area > 200:  # Lower threshold to capture smaller, distant objects
            contours_info.append({
                'contour': c,
                'area': area,
                'color_label': color_label,
                'rectangle_color': rectangle_color,
                'label_color': label_color
            })
    return contours_info

def draw_and_label_contours(frame, contours_info):
    """Draw and label the contours in the frame based on sorted order by size."""
    contours_info = sorted(contours_info, key=lambda x: x['area'], reverse=True)

    for i, info in enumerate(contours_info):
        c = info['contour']
        color_label = info['color_label']
        rectangle_color = info['rectangle_color']
        label_color = info['label_color']

        x, y, w, h = cv2.boundingRect(c)
        # Draw rectangle around contour
        cv2.rectangle(frame, (x, y), (x + w, y + h), rectangle_color, 2)

        # Display object order and color label on top
        order_label = f"{color_label} - Order {i + 1}"
        cv2.putText(frame, order_label, (x, y - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.6, label_color, 2)

        # Calculate centroid
        M = cv2.moments(c)
        if M['m00'] != 0:
            cX = int(M['m10'] / M['m00'])
            cY = int(M['m01'] / M['m00'])

            # Draw the centroid
            cv2.circle(frame, (cX, cY), 5, rectangle_color, -1)
            # Draw a vertical line at the X-coordinate
            cv2.line(frame, (cX, 0), (cX, frame.shape[0]), rectangle_color, 2)
            # Display the Y-coordinate
            cv2.putText(frame, f"Y: {cY}", (cX + 10, cY - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, rectangle_color, 2)

    return contours_info


def main():
    # Color ranges for red and green (can be adjusted dynamically)
    lower_red = np.array([97, 170, 70])
    upper_red = np.array([180, 255, 255])
    lower_green = np.array([27, 155, 3])
    upper_green = np.array([75, 255, 255])

    picam2 = initialize_camera()

    while True:
        frame = capture_frame(picam2, flip=True)  # Vertical flip applied here
        hsv = convert_to_hsv(frame)

        mask_red, mask_green = create_color_masks(hsv, lower_red, upper_red, lower_green, upper_green)

        # Store red and green contours with their information
        red_contours_info = find_and_store_contours(mask_red, "RED", (0, 0, 255), (0, 0, 255))
        green_contours_info = find_and_store_contours(mask_green, "GREEN", (0, 255, 0), (0, 255, 0))

        # Combine red and green contours into one list
        all_contours_info = red_contours_info + green_contours_info

        # Draw and label contours with distance order
        draw_and_label_contours(frame, all_contours_info)

        cv2.imshow("FRAME", frame)

        if cv2.waitKey(1) & 0xFF == 27:  # Exit on 'ESC'
            break

    picam2.stop()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()
