import cv2
import numpy as np

img = cv2.imread('docs/assets/eps_on_board.jpg')
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
bg = np.median(gray[0:20, -20:])

# Threshold
_, thresh = cv2.threshold(cv2.absdiff(gray, int(bg)), 30, 255, cv2.THRESH_BINARY)

# Morphological ops just to remove noise
kernel = np.ones((5,5), np.uint8)
opened = cv2.morphologyEx(thresh, cv2.MORPH_OPEN, kernel)
closed = cv2.morphologyEx(opened, cv2.MORPH_CLOSE, kernel)

# Find contour
contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
c = max(contours, key=cv2.contourArea)

# SIFT for mapping
main_img = cv2.imread('docs/assets/flatsat_board.jpg', cv2.IMREAD_GRAYSCALE)
sift = cv2.SIFT_create()
kp1, des1 = sift.detectAndCompute(main_img, None)
kp2, des2 = sift.detectAndCompute(gray, None)

flann = cv2.FlannBasedMatcher(dict(algorithm=1, trees=5), dict(checks=50))
matches = flann.knnMatch(des2, des1, k=2)

good = [m for m, n in matches if m.distance < 0.75 * n.distance]

if len(good) > 10:
    src_pts = np.float32([kp2[m.queryIdx].pt for m in good]).reshape(-1, 1, 2)
    dst_pts = np.float32([kp1[m.trainIdx].pt for m in good]).reshape(-1, 1, 2)
    M, _ = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)
    
    # Map the contour points
    pts = np.float32([pt[0] for pt in c]).reshape(-1, 1, 2)
    dst = cv2.perspectiveTransform(pts, M)
    
    # Simplify the mapped contour to a polygon with a tight epsilon
    epsilon = 0.005 * cv2.arcLength(dst, True)
    approx = cv2.approxPolyDP(dst, epsilon, True)
    
    points_str = " ".join([f"{int(pt[0][0])},{int(pt[0][1])}" for pt in approx])
    print("SVG Polygon points:")
    print(points_str)
    
    x_min = int(np.min(dst[:, 0, 0]))
    y_min = int(np.min(dst[:, 0, 1]))
    x_max = int(np.max(dst[:, 0, 0]))
    y_max = int(np.max(dst[:, 0, 1]))
    print(f"viewBox=\"{x_min} {y_min} {x_max-x_min} {y_max-y_min}\"")
else:
    print("SIFT failed")
