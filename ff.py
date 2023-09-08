import tkinter as tk
import random
import requests
from bs4 import BeautifulSoup
import re
from datetime import datetime

N = 2  # number of sensors

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

        # Initialize a label to display the current time at the top
        self.time_label = tk.Label(self.frame, text=self.get_current_time())
        self.time_label.grid(row=0, column=0, sticky=tk.W, pady=10, columnspan=2)  # span across 2 columns

        # Dynamically generate labels and buttons based on the sensor data
        self.labels = [tk.Label(self.frame, text=f"System {i+1}: Updating...") for i in range(N)]
        self.buttons = [
            tk.Button(self.frame, text="Check", command=lambda idx=i: self.check_temp(idx)) 
            for i in range(N)
        ]

        for i, (label, button) in enumerate(zip(self.labels, self.buttons)):
            label.grid(row=i+1, column=0, sticky=tk.W, pady=5)  # Offset the row by 1 to account for the time label
            button.grid(row=i+1, column=1, padx=10)

        # Start updating the temperatures and time
        self.update_temperatures()
        self.update_time()

    def check_temp(self, index):
        temp = getattr(self, f'sys{index+1}_temp')
        button = self.buttons[index]
        self.update_button_color(button, temp)

    def update_temperatures(self):
        info = extract_info_from_url(url)  # Re-fetch the data

        for i in range(N):
            setattr(self, f'sys{i+1}_temp', float(info['sensors'][i]['temperature']))
            self.labels[i].config(text=f"{info['sensors'][i]['name']}: {float(info['sensors'][i]['temperature'])}°C")
            self.check_temp(i)

        # Schedule the function to update temperatures again after 1 seconds
        self.root.after(1000, self.update_temperatures)

    def update_button_color(self, button, temperature):
        if temperature > 30:
            button.config(bg='red', fg='white')
        elif 26 <= temperature <= 30:
            button.config(bg='orange', fg='white')
        else:
            button.config(bg='green', fg='white')

    def get_current_time(self):
        # Fetches the current time and returns it as a formatted string
        return datetime.now().strftime('%Y-%m-%d %H:%M:%S')

    def update_time(self):
        # Updates the time label and schedules itself to run again after 1000 ms
        self.time_label.config(text=self.get_current_time())
        self.root.after(1000, self.update_time)

if __name__ == "__main__":
    root = tk.Tk()
    root.geometry("1000x800")
    root.option_add("*Font", "Helvetica 40")
    app = TemperatureApp(root)
    root.mainloop()
