import os

folder_path = os.path.dirname(os.path.abspath(__file__))

def detect_bom(data: bytes):
    if data.startswith(b'\xef\xbb\xbf'):
        return "utf-8-sig"
    elif data.startswith(b'\xff\xfe'):
        return "utf-16-le"
    elif data.startswith(b'\xfe\xff'):
        return "utf-16-be"
    elif data.startswith(b'\xff\xfe\x00\x00'):
        return "utf-32-le"
    elif data.startswith(b'\x00\x00\xfe\xff'):
        return "utf-32-be"
    return None

def try_read_with_encodings(path: str, encodings: list[str]) -> str | None:
    for enc in encodings:
        try:
            with open(path, encoding=enc) as f:
                f.read()
            return enc
        except:
            continue
    return None

target_extensions = ('.sql')

print(f"\n📂 Конвертация файлов в UTF-8 в папке: {folder_path}\n")

for filename in os.listdir(folder_path):
    full_path = os.path.join(folder_path, filename)
    if os.path.isfile(full_path) and filename.lower().endswith(target_extensions):
        try:
            with open(full_path, 'rb') as f:
                raw = f.read(10000)
                bom = detect_bom(raw)

            encoding = bom or try_read_with_encodings(full_path, ['utf-8', 'windows-1251', 'utf-16'])

            if not encoding:
                print(f"{filename}: ❌ Не удалось определить кодировку — пропуск")
                continue

            if encoding.lower().startswith("utf-8"):
                print(f"{filename}: уже в UTF-8 — пропуск")
                continue

            # Читать файл с исходной кодировкой и записать в UTF-8
            with open(full_path, encoding=encoding) as f:
                text = f.read()

            with open(full_path, 'w', encoding='utf-8') as f:
                f.write(text)

            print(f"{filename}: ✅ Конвертирован из {encoding} → utf-8")

        except Exception as e:
            print(f"{filename}: ❌ Ошибка — {e}")
