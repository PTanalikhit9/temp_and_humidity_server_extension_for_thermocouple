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
    temp_limit_match = re.search(r"TEMP_LIMIT\s*=\s*([-+]?\d*\.\d+|\d+)", text_content)
    sensor_matches = re.findall(r"Sensor (\d) \(([\w\s.]+)\)\s*Temperature:\s*([-+]?\d*\.\d+|\d+)", text_content)

    return {
        "temp_limit": float(temp_limit_match.group(1)) if temp_limit_match else None,
        "sensors": [
            {"id": int(match[0]), "name": match[1], "temperature": float(match[2])} for match in sensor_matches
        ]
    }

url = "http://192.168.0.51/"

info = extract_info_from_url(url)
print(info)
print(info['sensors'][0]['name'])
print(info['sensors'][0]['temperature'])
print(info['sensors'][1]['name'])
print(info['sensors'][1]['temperature'])
