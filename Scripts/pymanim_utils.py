import subprocess
import os

def frames_to_video(frames_dir, output_path, fps=30, frame_pattern="frame_%04d.png"):
    if not os.path.isdir(frames_dir):
        print(f"[ERROR] Directory not found: {frames_dir}")
        return False

    output_dir = os.path.dirname(output_path)
    if output_dir and not os.path.exists(output_dir):
        os.makedirs(output_dir, exist_ok=True)

    files = [f for f in os.listdir(frames_dir) if f.lower().endswith(('.png', '.jpg', '.bmp', '.exr'))]
    if not files:
        print(f"[ERROR] No image files found in: {frames_dir}")
        return False

    files.sort()
    first_file = files[0]

    if first_file.lower().endswith('.png'):
        ext = '.png'
    elif first_file.lower().endswith('.jpg'):
        ext = '.jpg'
    elif first_file.lower().endswith('.bmp'):
        ext = '.bmp'
    elif first_file.lower().endswith('.exr'):
        ext = '.exr'
    else:
        ext = '.png'

    base_name = first_file.rsplit('.', 1)[0]

    digits = ''
    for c in reversed(base_name):
        if c.isdigit():
            digits = c + digits
        else:
            break

    if digits:
        prefix = base_name[:-len(digits)]
        input_pattern = f"{prefix}%0{len(digits)}d{ext}"
    else:
        input_pattern = frame_pattern

    input_path = os.path.join(frames_dir, input_pattern)

    cmd = [
        'ffmpeg', '-y',
        '-framerate', str(fps),
        '-i', input_path,
        '-c:v', 'libx264',
        '-pix_fmt', 'yuv420p',
        '-crf', '18',
        '-preset', 'slow',
        output_path
    ]

    print(f"[INFO] Running ffmpeg: {' '.join(cmd)}")

    try:
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        if result.returncode == 0:
            print(f"[PASS] Video created: {output_path}")
            return True
        else:
            print(f"[ERROR] ffmpeg failed:\n{result.stderr}")
            return False
    except FileNotFoundError:
        print("[ERROR] ffmpeg not found. Please install ffmpeg and add it to PATH.")
        return False
    except subprocess.TimeoutExpired:
        print("[ERROR] ffmpeg timed out after 300 seconds.")
        return False


def get_frame_count(frames_dir):
    if not os.path.isdir(frames_dir):
        return 0
    return len([f for f in os.listdir(frames_dir) if f.lower().endswith(('.png', '.jpg', '.bmp', '.exr'))])
