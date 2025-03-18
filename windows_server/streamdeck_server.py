from flask import Flask, request, render_template_string
import configparser
import pyautogui
import subprocess
import os

app = Flask(__name__)
config = configparser.ConfigParser()

# 配置文件路径
CONFIG_FILE = "config.ini"

# 加载配置文件
def load_config():
    config.read(CONFIG_FILE)

# 保存配置文件
def save_config():
    with open(CONFIG_FILE, "w") as f:
        config.write(f)

# 执行按键操作
def execute_action(key_id):
    key = f"key{key_id}"
    if key in config:
        action_type = config[key]["type"]
        value = config[key]["value"]

        if action_type == "keyboard":
            keys = value.split("+")
            pyautogui.hotkey(*keys)
        elif action_type == "app":
            subprocess.Popen(value, shell=True)
        elif action_type == "text":
            pyautogui.write(value)
        elif action_type == "mouse":
            action, x, y = value.split(",")
            if action == "click":
                pyautogui.click(int(x), int(y))

# Web 控制台页面
@app.route("/")
def index():
    load_config()
    keys = []
    for section in config.sections():
        keys.append({
            "id": section,
            "type": config[section]["type"],
            "value": config[section]["value"]
        })
    return render_template_string("""
        <!DOCTYPE html>
        <html>
        <head><title>Stream Deck Config</title></head>
        <body>
            <h1>Stream Deck Configuration</h1>
            <form action="/save" method="POST">
                <table>
                    <tr><th>Key</th><th>Type</th><th>Value</th></tr>
                    {% for key in keys %}
                    <tr>
                        <td>{{ key.id }}</td>
                        <td>
                            <select name="{{ key.id }}_type">
                                <option value="keyboard" {% if key.type == "keyboard" %}selected{% endif %}>Keyboard</option>
                                <option value="app" {% if key.type == "app" %}selected{% endif %}>App</option>
                                <option value="text" {% if key.type == "text" %}selected{% endif %}>Text</option>
                                <option value="mouse" {% if key.type == "mouse" %}selected{% endif %}>Mouse</option>
                            </select>
                        </td>
                        <td><input type="text" name="{{ key.id }}_value" value="{{ key.value }}"></td>
                    </tr>
                    {% endfor %}
                </table>
                <button type="submit">Save</button>
            </form>
        </body>
        </html>
    """, keys=keys)

# 保存配置
@app.route("/save", methods=["POST"])
def save():
    for key in request.form:
        if key.endswith("_type"):
            key_id = key.replace("_type", "")
            config[key_id] = {
                "type": request.form[key],
                "value": request.form[f"{key_id}_value"]
            }
    save_config()
    return "Configuration saved!"

# 处理按键触发
@app.route("/trigger")
def trigger():
    key_id = request.args.get("key", type=int)
    execute_action(key_id)
    return f"Triggered key {key_id}"

if __name__ == "__main__":
    load_config()
    app.run(host="0.0.0.0", port=5000)