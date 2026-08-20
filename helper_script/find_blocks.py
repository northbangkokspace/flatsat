import cv2
import numpy as np

img = cv2.imread('docs/assets/eps_on_board.jpg')
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

# The bright regions are around 200-255 in brightness, darkened regions are darker.
_, thresh = cv2.threshold(gray, 180, 255, cv2.THRESH_BINARY)

# Morphological open to remove noise
kernel = np.ones((15, 15), np.uint8)
opened = cv2.morphologyEx(thresh, cv2.MORPH_OPEN, kernel)
# Morphological close to merge text/lines into solid blocks
closed = cv2.morphologyEx(opened, cv2.MORPH_CLOSE, kernel)

contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

scale_x = 6300 / 3891
scale_y = 5400 / 3242

print("Detected blocks scaled to 6300x5400:")
for c in contours:
    x, y, w, h = cv2.boundingRect(c)
    if w * h > 10000: # Filter small noise
        # Scale to flatsat_board.jpg
        sx = int(x * scale_x)
        sy = int(y * scale_y)
        sw = int(w * scale_x)
        sh = int(h * scale_y)
        print(f'<rect x="{sx}" y="{sy}" width="{sw}" height="{sh}" />')
