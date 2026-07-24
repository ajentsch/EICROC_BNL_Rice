from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd
import re
from collections import defaultdict

# 1. Setup path and targeting parameters
folder_path = Path("./vref_scan_results")  # Update with your folder path
target_column = "adc"  # Column name to calculate mean for
vrefs = []
valid_vrefs = []

threshold = 20

pixels_of_interest = [0, 1, 2, 3, 32, 33, 34, 35, 64, 65, 66, 67, 96, 97, 98, 99]
adc_arrays_dict = {f"pix_{i}": [] for i in pixels_of_interest}
vref_dict = {f"pix_{i}": [] for i in pixels_of_interest}
mean_list = []

# glob('*.csv') streams file paths without loading everything into memory
for file_path in folder_path.glob("*.csv"):
    try:
        # Extracts the first sequence of digits and converts base-10 to int
        matches = re.findall(r'\d+', file_path.name)
        if matches:
            vref = int(matches[0]) # Take the first matched string from the list
        else:
            continue
        vrefs.append(vref)
        df = pd.read_csv(file_path)
        df = df[~df['event'].astype(str).str.contains('event', na=False)]

        # initialize each row
        df['event'] = df['event'].astype(int)
        df['column'] = df['column'].astype(int)
        df['row'] = df['row'].astype(int)
        df['timebin'] = df['timebin'].astype(int)

        df['adc'] = pd.to_numeric(df['adc'], errors='coerce')

        # check max number of cols, rows, and events
        event_total = df['event'].max()
        col_total = df['column'].max()
        row_total = df['row'].max()
        timebin_total = df['timebin'].max()

        pixels_per_event = (col_total+1)*(row_total+1) # +1 accounts for 0 indexing

        # Create pixel_no vectorially (MUCH faster than iterrows)
        df['pixel_no'] = df['row'] * 32 + df['column']

        # Filter only rows matching your pixels of interest
        filtered_df = df[df['pixel_no'].isin(pixels_of_interest)]

        # Group by pixel_no and calculate the mean for each pixel instantly!
        pixel_means = filtered_df.groupby('pixel_no')['adc'].mean()

        # Store in your dictionary
        for pixel_no in pixels_of_interest:
            if pixel_no in pixel_means:
                vref_dict[f"pix_{pixel_no}"].append((vref, pixel_means[pixel_no]))
        
        adc_arrays_dict = {f"pix_{i}": [] for i in pixels_of_interest}

    except Exception as e:
        print(f"Error reading {file_path.name}: {e}")

# 4. Plot the results
plt.figure(figsize=(10, 5))
for pixel in pixels_of_interest:
    data = vref_dict[f"pix_{pixel}"]
    sorted_data = sorted(data, key=lambda x: x[0])
    vref, mean = zip(*sorted_data)
    plt.scatter(
        vref,
        mean,
        marker="o",
        label=f"pixel {pixel}"
    )
    below_thresh_idx = next((i for i, x in enumerate(mean) if x <= threshold), None)
    valid_vref = vref[below_thresh_idx]
    valid_vrefs.append(valid_vref)

plt.plot(
    vref,
    [threshold for i in vref],
    linestyle='-',
    color='black',
    label='20 ADC count target'
)

valid_vref_data = { 'pixel':pixels_of_interest,
                 'vref':valid_vrefs }

valid_vref_df = pd.DataFrame(valid_vref_data)
valid_vref_df.to_csv('vref_scan_values.csv', index=False)

plt.title(f"Vref ADC Scan")
plt.xlabel("Vref")
plt.ylabel(f"Mean ADC (counts)")
plt.xticks(rotation=45, ha="right")  # Rotate labels for readability
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()

# Save plot to disk and render
plt.savefig("vref_scan_results.png", dpi=300)
plt.legend()
plt.show()