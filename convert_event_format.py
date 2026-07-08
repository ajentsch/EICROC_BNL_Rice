import pandas as pd
from collections import defaultdict

input_filename = "output.csv"
output_filename = f"converted_{input_filename}"

# load data, delete any strings
df = pd.read_csv(input_filename)
df = df[~df['event'].astype(str).str.contains('event', na=False)]

# initialize each row
df['event'] = df['event'].astype(int)
df['column'] = df['column'].astype(int)
df['row'] = df['row'].astype(int)
df['timebin'] = df['timebin'].astype(int)

print("df contains:")
print(df.head())

# 2. Populate the dictionary directly from the DataFrame rows
data = defaultdict(lambda: defaultdict(dict))

for _, row in df.iterrows():
    evt = row["event"]
    col = row["column"]
    p_row = row["row"]
    tbin = row["timebin"]
    
    # save triplet of adc, tdc, hit as a str triplet for later modification
    data[evt][(col, p_row)][tbin] = (str(row["tdc"]), str(row["adc"]), str(row["hit"]))

print(data)

# open output file and write data in required semicolon format
with open(output_filename, "w", newline="") as outfile:
    for evt in sorted(data.keys()):
        print(evt)
        for p_row in range(0,32):
            for col in range(0,32):
                pixel_data = data[evt][(col, p_row)]
                print(pixel_data)
                for tbin in sorted(pixel_data.keys()):
                    tdc, adc, hit = pixel_data[tbin]
                    if tbin == 0: output_line = [str(evt), tdc, adc, hit]
                    else: output_line.extend([tdc, adc, hit])
                outfile.write(";".join(output_line) + "\n")

print(f"\nFile converted successfully, saved to {output_filename}")