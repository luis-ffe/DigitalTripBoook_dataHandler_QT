# DataHandler - InfluxDB Data Fetcher

This application fetches speed, charge, and autonomy level data from InfluxDB and displays it in the terminal.

## Setup

### 1. Set InfluxDB Credentials

You can set your InfluxDB credentials in two ways:

#### Option A: Environment Variables (Recommended)
```bash
export INFLUX_TOKEN="your_actual_influx_token_here"
export INFLUX_ORG="your_org_name"
export INFLUX_BUCKET="your_bucket_name"
```

#### Option B: Edit the source code
Edit `influxdbclient.cpp` and replace:
- `YOUR_INFLUX_TOKEN_HERE` with your actual token
- `YOUR_ORG_HERE` with your organization name  
- `YOUR_BUCKET_HERE` with your bucket name

### 2. Build and Run

```bash
# Build the project
cmake -B build
cmake --build build

# Run the application
./build/appdataHandler.app/Contents/MacOS/appdataHandler
```

## Expected Output

The application will:
1. Connect to InfluxDB every 5 seconds
2. Fetch the latest values for speed, charge, and autonomyLevel
3. Display detailed parsing information in the terminal
4. Show a summary of current values

Example output:
```
=== INFLUXDB RESPONSE ANALYSIS ===
Total lines received: 4
CSV Headers: ("result", "_start", "_stop", "_time", "_value", "_field", "_measurement")
=== PARSED VALUE ===
Field: speed
Value: 65.5
Time: 2025-01-17T10:30:00Z
*** UPDATED SPEED: 65.5 km/h

=== CURRENT VALUES SUMMARY ===
Speed:          65.5 km/h
Charge:         78.2 %
Autonomy Level: 3
===============================
```

## Troubleshooting

- If you see "WARNING: Please set your InfluxDB credentials", update your credentials using one of the methods above
- If you get network errors, check your internet connection and InfluxDB credentials
- The application fetches data every 5 seconds - be patient for the first update
