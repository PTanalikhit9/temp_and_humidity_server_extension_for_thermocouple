import tkinter as tk
import random
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

class TemperatureApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Temperature Monitor")
        
        self.frame = tk.Frame(self.root, padx=10, pady=10)
        self.frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Initialize labels for each system
        self.sys1_label = tk.Label(self.frame, text="System 1: Updating...")
        self.sys2_label = tk.Label(self.frame, text="System 2: Updating...")
        self.sys3_label = tk.Label(self.frame, text="System 3: Updating...")
        
        # Initialize buttons for each system
        self.sys1_button = tk.Button(self.frame, text="Check", command=self.check_temp1)
        self.sys2_button = tk.Button(self.frame, text="Check", command=self.check_temp2)
        self.sys3_button = tk.Button(self.frame, text="Check", command=self.check_temp3)
        
        self.sys1_label.grid(row=0, column=0, sticky=tk.W, pady=5)
        self.sys1_button.grid(row=0, column=1, padx=10)
        self.sys2_label.grid(row=1, column=0, sticky=tk.W, pady=5)
        self.sys2_button.grid(row=1, column=1, padx=10)
        self.sys3_label.grid(row=2, column=0, sticky=tk.W, pady=5)
        self.sys3_button.grid(row=2, column=1, padx=10)
        
        # Start updating the temperatures
        self.update_temperatures()
        
    def check_temp1(self):
        self.update_button_color(self.sys1_button, self.sys1_temp)
    
    def check_temp2(self):
        self.update_button_color(self.sys2_button, self.sys2_temp)
    
    def check_temp3(self):
        self.update_button_color(self.sys3_button, self.sys3_temp)

    def update_temperatures(self):
        info = extract_info_from_url(url)  # Re-fetch the data

        # For demonstration purposes, generate random temperatures
        self.sys1_temp = float(info['sensors'][0]['temperature'])
        self.sys2_temp = info['sensors'][1]['temperature']
        self.sys3_temp = random.randint(20, 40)
        
        self.sys1_label.config(text=f"MOT coil: {self.sys1_temp}°C")
        self.sys2_label.config(text=f"Fesh. coil: {self.sys2_temp}°C")
        self.sys3_label.config(text=f"Random: {self.sys3_temp}°C")
        
        # Update button colors automatically based on temperature
        self.check_temp1()
        self.check_temp2()
        self.check_temp3()

        # Schedule the function to update temperatures again after 2 seconds
        self.root.after(1000, self.update_temperatures)
    
    def update_button_color(self, button, temperature):
        if temperature > 30:
            button.config(bg='red', fg='white')
        elif 26 <= temperature <= 30:
            button.config(bg='orange', fg='white')
        else:
            button.config(bg='green', fg='white')

if __name__ == "__main__":
    root = tk.Tk()
    root.geometry("500x500")
    root.option_add("*Font", "Arial 25")
    app = TemperatureApp(root)
    root.mainloop()
