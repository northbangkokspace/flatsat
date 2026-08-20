import cv2
import numpy as np

img = cv2.imread('docs/assets/eps_on_board.jpg')
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
# The background in the ascii looks like uniform light grey or white, 
# but the first pixel was 80 (dark grey).
# Let's use a Canny edge detector and morph close to get the solid shape
edges = cv2.Canny(gray, 20, 100)
kernel = np.ones((50,50), np.uint8)
closed = cv2.morphologyEx(edges, cv2.MORPH_CLOSE, kernel)

# Find contours
contours, _ = cv2.findContours(closed, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
if not contours:
    print("No contours found")
    exit()
    
c = max(contours, key=cv2.contourArea)
epsilon = 0.005 * cv2.arcLength(c, True)
approx = cv2.approxPolyDP(c, epsilon, True)

# SIFT to find mapping to flatsat_board.jpg
main_img = cv2.imread('docs/assets/flatsat_board.jpg', cv2.IMREAD_GRAYSCALE)
sift = cv2.SIFT_create()
kp1, des1 = sift.detectAndCompute(main_img, None)
kp2, des2 = sift.detectAndCompute(gray, None)

flann = cv2.FlannBasedMatcher(dict(algorithm=1, trees=5), dict(checks=50))
matches = flann.knnMatch(des2, des1, k=2)

good = []
for m, n in matches:
    if m.distance < 0.75 * n.distance:
        good.append(m)

if len(good) > 10:
    src_pts = np.float32([kp2[m.queryIdx].pt for m in good]).reshape(-1, 1, 2)
    dst_pts = np.float32([kp1[m.trainIdx].pt for m in good]).reshape(-1, 1, 2)
    M, _ = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)
    
    # Transform the polygon points
    pts = np.float32([pt[0] for pt in approx]).reshape(-1, 1, 2)
    dst = cv2.perspectiveTransform(pts, M)
    
    points_str = " ".join([f"{int(pt[0][0])},{int(pt[0][1])}" for pt in dst])
    print("SVG Polygon points:")
    print(points_str)
    
    # Also get bounding box for viewBox
    x_min = int(np.min(dst[:, 0, 0]))
    y_min = int(np.min(dst[:, 0, 1]))
    x_max = int(np.max(dst[:, 0, 0]))
    y_max = int(np.max(dst[:, 0, 1]))
    print(f"viewBox=\"{x_min} {y_min} {x_max-x_min} {y_max-y_min}\"")
else:
    print("SIFT failed")
