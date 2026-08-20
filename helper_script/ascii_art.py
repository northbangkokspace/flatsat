import cv2

img = cv2.imread('docs/assets/eps_on_board.jpg', cv2.IMREAD_GRAYSCALE)
# Resize to 80x40 for terminal output
resized = cv2.resize(img, (80, 40))

# Convert to ascii
chars = " .:-=+*#%@"
ascii_str = ""
for row in resized:
    for pixel in row:
        ascii_str += chars[pixel // 26]
    ascii_str += "\n"
print(ascii_str)
