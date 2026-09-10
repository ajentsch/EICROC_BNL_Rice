from pathlib import Path
import matplotlib.pyplot as plt
import pandas as pd
import re
from collections import defaultdict
import sys
import numpy as np

folder_path = Path("./retried_s_curves")
threshes = []

threshold = 20

pixels_of_interest = [0, 1, 2, 3, 32, 33, 34, 35, 64, 65, 66, 67, 96, 97, 98, 99]
efficiency_dict = {f"pix_{i}": [] for i in pixels_of_interest}
mean_list = []

# glob('*.csv') streams file paths without loading everything into memory
for file_path in folder_path.glob("*.csv"):
    try:
        # extracts the first sequence of digits and converts base-10 to int
        matches = re.findall(r'\d+', file_path.name)
        if matches:
            thresh = int(matches[0]) # Take the first matched string from the list
        else:
            continue
        threshes.append(thresh)
        df = pd.read_csv(file_path)
        df = df[~df['event'].astype(str).str.contains('event', na=False)]

        # initialize each row
        df['event'] = df['event'].astype(int)
        df['column'] = df['column'].astype(int)
        df['row'] = df['row'].astype(int)
        df['timebin'] = df['timebin'].astype(int)

        df['tdc'] = pd.to_numeric(df['tdc'], errors='coerce')
        df['hit'] = pd.to_numeric(df['hit'], errors='coerce')

        # check max number of cols, rows, and events
        event_total = df['event'].max()
        col_total = df['column'].max()
        row_total = df['row'].max()
        timebin_total = df['timebin'].max()

        pixels_per_event = (col_total+1)*(row_total+1) # +1 accounts for 0 indexing
        df['pixel_no'] = df['row'] * 32 + df['column']
        filtered_df = df[df['pixel_no'].isin(pixels_of_interest)]

        hitbit = 1

        # group by pixel_no
        '''pixel_means = filtered_df.groupby(['pixel_no','event'])
        good_event = True
        for group, val in pixel_means:
            event_no = group[1]
            pixel_no = group[0]
            print(group)
            print(val)
            if 1 not in val['hit'].values: good_event = False
            if (val['hit'] == 1).sum() > 1: good_event = False
            if (val['tdc'] > 0).sum() > 1: good_event = False
            print(good_event)
            sys.exit()
            if good_event:
                good_events_dict[f"pix_{pixel_no}"].append((thresh, efficiency))'''
        
        event_stats = filtered_df.groupby(['pixel_no', 'event']).agg(
            hit_count=('hit', lambda x: (x == 1).sum()),
            tdc_count=('tdc', lambda x: (x > 0).sum())
        )

        event_stats['is_good'] = (event_stats['hit_count'] == 1) & (event_stats['tdc_count'] <= 1)

        pixel_summary = event_stats.groupby('pixel_no')['is_good'].agg(
            good_events='sum',
            total_events='count',
            efficiency='mean'  # mean of booleans is (good_events / total_events)
        ).reset_index()

        if not pixel_summary.empty:
            for pixel_no, efficiency in zip(pixel_summary['pixel_no'], pixel_summary['efficiency']):
                pix_key = f"pix_{int(pixel_no)}"
                if pix_key in efficiency_dict:
                    efficiency_dict[pix_key].append((thresh, efficiency))

    except Exception as e:
        print(f"Error reading {file_path.name}: {e}")

# 4. Plot the results
plt.figure(figsize=(10, 5))
counter = 0
for pixel in pixels_of_interest:
    data = efficiency_dict[f"pix_{pixel}"]
    sorted_data = sorted(data, key=lambda x: x[0])
    thresh, efficiency = zip(*sorted_data)

    plt.scatter(
        thresh,
        efficiency,
        marker="o",
        label=f"pixel {pixel}"
    )
    counter += 1

plt.title(f"Threshold Scan")
plt.xlabel("Vthresh")
plt.ylabel(f"Efficiency")
plt.xticks(rotation=45, ha="right")
plt.grid(True, linestyle="--", alpha=0.6)
plt.tight_layout()
plt.savefig("vthresh_scan_results.png", dpi=300)
plt.legend()
plt.show()