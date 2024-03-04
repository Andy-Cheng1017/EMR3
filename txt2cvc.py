import csv

def txt_to_csv(txt_file, csv_file):
    try:
        with open(txt_file, 'r') as txt:
            lines = txt.readlines()

        if len(lines) < 5:
            raise ValueError("Invalid TXT file format")

        clock_line = [line for line in lines if "#CLOCK" in line]
        size_line = [line for line in lines if "#SIZE" in line]

        if not clock_line or not size_line:
            raise ValueError("Missing CLOCK or SIZE information in the TXT file")

        clock = clock_line[0].split('=')[1].strip()
        size = size_line[0].split('=')[1].strip()

        csv_content = []

        data_lines = lines[4:]  # Skip the first 4 lines containing metadata

        for line in data_lines:
            voltage = line.strip()
            csv_content.append([voltage, '0', '0'])

        with open(csv_file, 'w', newline='') as csv_output:
            csv_writer = csv.writer(csv_output)
            csv_writer.writerow(['#CLOCK=' + clock])
            csv_writer.writerow(['#SIZE=' + size])
            csv_writer.writerow(['#PROBE=1'])
            csv_writer.writerow(['#VOLTDIV=5'])
            csv_writer.writerows(csv_content)

        print(f"Conversion successful. CSV file '{csv_file}' created.")
    except Exception as e:
        print(f"Error: {e}")

# 請提供您的TXT檔案和欲輸出的CSV檔案名稱
txt_filename = '66.txt'
csv_filename = '66.csv'

# 呼叫函數執行轉換
txt_to_csv(txt_filename, csv_filename)
