from PIL import Image, ImageDraw, ImageFont
import os

# Create assets/images directory if it doesn't exist
os.makedirs('assets/images', exist_ok=True)

# Create background (1280x720, black with subtle grid)
bg = Image.new('RGB', (1280, 720), (5, 5, 5))
draw = ImageDraw.Draw(bg)
# Draw subtle grid lines
for x in range(0, 1280, 64):
    draw.line([(x, 0), (x, 720)], fill=(15, 15, 15), width=1)
for y in range(0, 720, 64):
    draw.line([(0, y), (1280, y)], fill=(15, 15, 15), width=1)
bg.save('assets/images/bg_main.png')

# Create icon templates (64x64 white icons on transparent background)
icon_size = 128

def create_icon(filename, shape_type):
    img = Image.new('RGBA', (icon_size, icon_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    if shape_type == 'games':
        # Controller icon (simplified)
        draw.rectangle([20, 40, 108, 88], outline=(255, 255, 255), width=4)
        draw.ellipse([10, 50, 30, 70], outline=(255, 255, 255), width=3)
        draw.ellipse([98, 50, 118, 70], outline=(255, 255, 255), width=3)
        draw.ellipse([45, 52, 55, 62], fill=(255, 255, 255))
        draw.ellipse([73, 52, 83, 62], fill=(255, 255, 255))
    elif shape_type == 'tools':
        # Wrench icon
        draw.ellipse([40, 20, 70, 50], outline=(255, 255, 255), width=4)
        draw.rectangle([50, 45, 60, 100], fill=(255, 255, 255))
        draw.rectangle([35, 85, 75, 95], fill=(255, 255, 255))
    elif shape_type == 'media':
        # Play button
        draw.regular_polygon((64, 64, 50), 3, rotation=30, fill=(255, 255, 255))
    elif shape_type == 'settings':
        # Gear icon
        draw.ellipse([44, 44, 84, 84], outline=(255, 255, 255), width=6)
        for angle in range(0, 360, 45):
            import math
            rad = math.radians(angle)
            x1 = 64 + 30 * math.cos(rad)
            y1 = 64 + 30 * math.sin(rad)
            x2 = 64 + 45 * math.cos(rad)
            y2 = 64 + 45 * math.sin(rad)
            draw.line([(x1, y1), (x2, y2)], fill=(255, 255, 255), width=6)
    
    img.save(f'assets/images/{filename}')

create_icon('icon_games.png', 'games')
create_icon('icon_tools.png', 'tools')
create_icon('icon_media.png', 'media')
create_icon('icon_settings.png', 'settings')

print("✓ Created background and icon assets")
