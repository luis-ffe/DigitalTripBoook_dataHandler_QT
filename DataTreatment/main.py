from influxdb_client_3 import InfluxDBClient3, Point
import pandas as pd
import math

# InfluxDB connection
client = InfluxDBClient3(
    "https://eu-central-1-1.aws.cloud2.influxdata.com",
    database="jetracer",
    token="rYtXXREgOrb0Kd5DSkA4b--qI9AC1gHvIGfNK90Ne0yGHsIDAYkvyxKzgxDLonwTVhzclF8ZZoVk7R9atXeHbQ=="  # ← Replace with your actual token or env variable
)

# Topic mapping: raw → treated
topics = {
    "Vehicle/1/Speed": "Vehicle/1/qt/speed",
    "Vehicle/1/Powertrain/TractionBattery/StateOfCharge": "Vehicle/1/qt/charge",
    "Vehicle/1/ADAS/ActiveAutonomyLevel": "Vehicle/1/qt/autonomy_level"
}

# How far back to look for raw data
time_range = "10 days"

for source_topic, target_topic in topics.items():
    print(f"Processing topic: {source_topic} → {target_topic}")

    # Query raw data
    query = f"""
    SELECT *
    FROM "{source_topic}"
    WHERE time >= now() - interval '{time_range}'
    """
    table = client.query(query)
    df = table.to_pandas()

    if df.empty:
        print("No data found.")
        continue

    # Parse time and extract numerical value from string
    df['time'] = pd.to_datetime(df['time'])
    df.set_index('time', inplace=True)
    df['value'] = df['value'].astype(str).str.extract(r'([\d.]+)').astype(float)
    df = df.dropna(subset=['value'])

    # Convert RPM to m/s for speed
    if source_topic == "Vehicle/1/Speed":
        df['value'] = df['value'] * 0.067 * 2 * math.pi / 60

    # Normalize battery SoC to 0–100
    elif source_topic == "Vehicle/1/Powertrain/TractionBattery/StateOfCharge":
        df['value'] = df['value'].clip(lower=0, upper=100)

    # Resample to 1-second intervals
    df_resampled = df['value'].resample('1s').mean().dropna()

    # Write to InfluxDB
    count = 0
    for ts, value in df_resampled.items():
        point = Point(target_topic).time(ts).field("value", value)
        client.write(org="SEA:ME", record=point)
        count += 1

    print(f"Wrote {count} points to {target_topic}")

# Cleanup
client.close()
print("✅ Done.")
