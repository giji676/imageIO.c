from PIL import Image

width, height = 100, 100

img = Image.new("RGB", (width, height))
pixels = img.load()

for y in range(height):
    value = int(y * 255 / (height - 1))
    for x in range(width):
        pixels[x, y] = (value, value, value)

img.save("gradient_rows.png")
