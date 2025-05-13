import os

folder_path = os.path.dirname(os.path.abspath(__file__))

def detect_bom(data: bytes):
    if data.startswith(b'\xef\xbb\xbf'):
        return "utf-8-sig"  # UTF-8 with BOM
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

results = []

for filename in os.listdir(folder_path):
    full_path = os.path.join(folder_path, filename)
    if os.path.isfile(full_path) and filename.lower().endswith(target_extensions):
        try:
            with open(full_path, 'rb') as f:
                raw = f.read(10000)
                bom = detect_bom(raw)
            if bom:
                encoding = bom
                source = "по BOM"
            else:
                encoding = try_read_with_encodings(full_path, ['utf-8', 'windows-1251', 'utf-16'])
                source = "перебором" if encoding else None

            # if encoding and encoding.lower().startswith("utf-8"):
            #     continue  # Пропустить utf-8 и utf-8-sig

            if encoding:
                results.append(f"{filename}: {encoding} ({source})")
            else:
                results.append(f"{filename}: ❌ не удалось определить")

        except Exception as e:
            results.append(f"{filename}: ❌ Ошибка — {e}")

# Вывод на экран
print("\nФайлы с кодировкой, отличной от UTF-8:\n")
for line in results:
    print(line)

# Сохранение в файл
with open("encoding_report.txt", "w", encoding="utf-8") as report:
    report.write("Файлы с кодировкой, отличной от UTF-8:\n\n")
    for line in results:
        report.write(line + "\n")
