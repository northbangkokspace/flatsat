import cv2
import numpy as np

img = cv2.imread('docs/assets/eps_on_board.jpg')

# Assume background is white. Create a mask of non-white pixels.
# White is [255, 255, 255]. Let's say anything > [240, 240, 240] is background.
lower_white = np.array([230, 230, 230], dtype=np.uint8)
upper_white = np.array([255, 255, 255], dtype=np.uint8)
bg_mask = cv2.inRange(img, lower_white, upper_white)

# The foreground is the inverse of the background
fg_mask = cv2.bitwise_not(bg_mask)

# Find contours of the foreground
contours, _ = cv2.findContours(fg_mask, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)
if not contours:
    print("No contours found")
    exit()

# Filter small noise contours and find the main shape
valid_contours = [c for c in contours if cv2.contourArea(c) > 50000]

if not valid_contours:
    print("No valid large contours found.")
    exit()

# If it's a single contiguous shape, we can get the polygon of the largest one.
# Or if it's multiple parts, we can take the convex hull of all valid contours.
# Since the user says "it isn't a simple rectangle", let's get the exact polygon of the largest one.
c = max(valid_contours, key=cv2.contourArea)

epsilon = 0.01 * cv2.arcLength(c, True)
approx = cv2.approxPolyDP(c, epsilon, True)

print(f"Foreground polygon has {len(approx)} vertices:")
for pt in approx:
    print(f"{pt[0][0]}, {pt[0][1]}")

# SIFT mapping
main_img = cv2.imread('docs/assets/flatsat_board.jpg', cv2.IMREAD_GRAYSCALE)
gray = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)
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
    
    pts = np.float32([pt[0] for pt in approx]).reshape(-1, 1, 2)
    dst = cv2.perspectiveTransform(pts, M)
    
    print("Mapped SVG Polygon points:")
    points_str = " ".join([f"{int(pt[0][0])},{int(pt[0][1])}" for pt in dst])
    print(points_str)
else:
    print("SIFT failed")
