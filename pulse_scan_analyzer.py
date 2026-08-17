from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd
import re
from collections import defaultdict
import sys
import numpy as np

# 1. Setup path and targeting parameters
folder_path = Path("./pulse_results")  # Update with your folder path
target_column = "adc"  # Column name to calculate mean for
pulses = []
valid_pulses = []

threshold = 20

pixels_of_interest = [0, 1, 2, 3, 32, 33, 34, 35, 64, 65, 66, 67, 96, 97, 98, 99]
adc_arrays_dict = {f"pix_{i}": [] for i in pixels_of_interest}
pulse_dict = {f"pix_{i}": [] for i in pixels_of_interest}
mean_list = []

# glob('*.csv') streams file paths without loading everything into memory
for file_path in folder_path.glob("*.csv"):
    try:
        # Extracts the first sequence of digits and converts base-10 to int
        matches = re.findall(r'\d+', file_path.name)
        if matches:
            original_pulse = int(matches[0]) # Take the first matched string from the list
            pulse = 63 - original_pulse
        else:
            continue
        pulses.append(pulse)
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
        pixel_means = filtered_df.groupby(['pixel_no','timebin'])['adc'].mean()
        max_per_pixel = filtered_df.loc[filtered_df.groupby('pixel_no')['adc'].idxmax()]

        for pixel_no in pixels_of_interest:
            # Filter for the specific pixel
            pixel_row = max_per_pixel[max_per_pixel['pixel_no'] == pixel_no]
            
            if not pixel_row.empty:
                # Extract scalar values using .item()
                adc_maxima = pixel_row['adc'].item()
                max_tbin = pixel_row['timebin'].item()
                
                pulse_dict[f"pix_{pixel_no}"].append((pulse, adc_maxima, max_tbin))

    except Exception as e:
        print(f"Error reading {file_path.name}: {e}")

pedestal_df = pd.read_csv("/Users/rkfuentes/Documents/phd/research/BNL_summer_2026/EICROC_analysis/raw_data_csvs/vref_scan_values.csv")
vrefs = pedestal_df['vref']

# 4. Plot the results
plt.figure(figsize=(10, 5))
counter = 0
for pixel in pixels_of_interest:
    vref = pedestal_df.loc[pedestal_df['pixel'] == pixel, 'vref'].item()
    data = pulse_dict[f"pix_{pixel}"]
    sorted_data = sorted(data, key=lambda x: x[0])
    pulse, max_adc, max_tbin = zip(*sorted_data)

    max_adc_arr = np.array(max_adc, dtype=float).flatten()
    vrefs = [vref for i in max_adc_arr]
    print(vrefs)
    pedestal_subtracted = max_adc_arr - vrefs

    plt.scatter(
        pulse,
        pedestal_subtracted,
        marker="o",
        label=f"pixel {pixel}"
    )
    counter += 1

plt.title(f"Pulse ADC Scan With Artificial Pedestal Subtraction")
plt.xlabel("Pulse")
plt.ylabel(f"Max Avg ADC (counts), tbin 3 for all pixels")
plt.xticks(rotation=45, ha="right")  # Rotate labels for readability
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()

# Save plot to disk and render
plt.savefig("pulse_scan_results.png", dpi=300)
plt.legend()
plt.show()