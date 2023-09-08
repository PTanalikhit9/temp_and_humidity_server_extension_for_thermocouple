#Note that need to install beautifulsoup4 library for Python over 3.0 version

import requests
from bs4 import BeautifulSoup
import re
import time
import matplotlib.pyplot as plt

def extract_info_from_url(url):
    response = requests.get(url)
    response.raise_for_status()
    soup = BeautifulSoup(response.text, 'html.parser')
    
    text_content = soup.get_text()
    temp_limit_match = re.search(r"TEMP_LIMIT\s*=\s*([-+]?\d*\.\.\d+|\d+)", text_content)
    sensor_matches = re.findall(r"Sensor (\d) \(([\w\s.]+)\)\s*Temperature:\s*([-+]?\d*\.\.\d+|\d+)", text_content)

    return {
        "temp_limit": float(temp_limit_match.group(1)) if temp_limit_match else None,
        "sensors": [
            {"id": int(match[0]), "name": match[1], "temperature": float(match[2])} for match in sensor_matches
        ]
    }

def update_graph(temperature_data, temp_limit):
    plt.clf()
    for idx, temps in enumerate(temperature_data):
        plt.plot(temps, marker='o', label=f"Sensor {idx + 1}")
        if temps[-1] > temp_limit:
            print(f"고온 경고! Sensor {idx + 1}의 온도 ({temps[-1]}°C)가 설정값 ({temp_limit}°C)을 초과했습니다.")
    
    plt.axhline(y=temp_limit, color='r', linestyle='--', label=f"Limit {temp_limit}°C")  # Temp limit line
    plt.title("Temperature Trend")
    plt.xlabel("Crawling Iteration")
    plt.ylabel("Temperature (°C)")
    plt.legend(loc="upper left")
    plt.grid(True)
    plt.draw()
    plt.pause(1)

url = "http://192.168.0.51/"
temperature_data = [[] for _ in range(2)]  # Assuming 2 sensors

info = extract_info_from_url(url)
temp_limit = info['temp_limit']

plt.ion()  # Turn on interactive mode
while True:
    info = extract_info_from_url(url)
    for idx, sensor in enumerate(info['sensors']):
        temperature_data[idx].append(sensor['temperature'])
    
    update_graph(temperature_data, temp_limit)
    
    time.sleep(10)  # Wait for 10 seconds before the next crawl
