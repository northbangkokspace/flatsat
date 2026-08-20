import cv2
import numpy as np

img = cv2.imread('docs/assets/eps_on_board.jpg')
# check corner colors
print("Corners:", img[0,0], img[0,-1], img[-1,0], img[-1,-1])

# Check for purely black/white or uniform background
# We can use Canny edge detection and find the bounding polygon of all edges
edges = cv2.Canny(img, 50, 150)
contours, _ = cv2.findContours(edges, cv2.RETR_EXTERNAL, cv2.CHAIN_APPROX_SIMPLE)

if contours:
    # Combine all contours
    all_pts = np.vstack(contours)
    hull = cv2.convexHull(all_pts)
    epsilon = 0.02 * cv2.arcLength(hull, True)
    approx = cv2.approxPolyDP(hull, epsilon, True)
    print("Convex Hull Polygon:", approx.reshape(-1, 2).tolist())

    # Get bounding rect of the non-background part
    x, y, w, h = cv2.boundingRect(all_pts)
    print(f"Bounding Rect: x={x}, y={y}, w={w}, h={h}")

# Also try SIFT against flatsat_board.jpg again to find the polygon
main_img = cv2.imread('docs/assets/flatsat_board.jpg')
sift = cv2.SIFT_create()
kp1, des1 = sift.detectAndCompute(main_img, None)
kp2, des2 = sift.detectAndCompute(img, None)

FLANN_INDEX_KDTREE = 1
flann = cv2.FlannBasedMatcher(dict(algorithm=FLANN_INDEX_KDTREE, trees=5), dict(checks=50))
matches = flann.knnMatch(des2, des1, k=2)

good = []
for m, n in matches:
    if m.distance < 0.75 * n.distance:
        good.append(m)

if len(good) > 10:
    src_pts = np.float32([kp2[m.queryIdx].pt for m in good]).reshape(-1, 1, 2)
    dst_pts = np.float32([kp1[m.trainIdx].pt for m in good]).reshape(-1, 1, 2)
    M, _ = cv2.findHomography(src_pts, dst_pts, cv2.RANSAC, 5.0)
    
    h, w = img.shape[:2]
    pts = np.float32([[0,0], [0,h-1], [w-1,h-1], [w-1,0]]).reshape(-1,1,2)
    dst = cv2.perspectiveTransform(pts, M)
    print("Mapped Corners on flatsat_board:", dst.reshape(-1, 2).tolist())
