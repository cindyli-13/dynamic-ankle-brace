import threading, random
from gui import *
import socket
import time

ESP32_IP_ADDR = "192.168.4.1"
ESP32_LISTEN_PORT = 4242
UDP_LISTEN_PORT = 4243
UDP_SOCKET_TIMEOUT_S = 5

default_config = {
    "inv_thresh": 300,
    "act_time": 2500,
    "idle_var": 1.2,
    "idle_time": 60000,
}


string_to_state = {
    "kCalibrating": State.Calibrating,
    "kIdle": State.Idle,
    "kActive": State.Active,
    "kActuated": State.Actuated,
}


def send_tcp_data(data):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.connect((ESP32_IP_ADDR, ESP32_LISTEN_PORT))
        print(f"Connected to {ESP32_IP_ADDR}:{ESP32_LISTEN_PORT}")
        sock.sendall(data.encode("utf-8"))
        print(f"Sent: {data}")


def calibration_requested_callback():
    send_tcp_data("CALIBRATE")


def config_update_requested_callback(
    inversion_threshold_dps,
    actuated_timeout_s,
    idle_variance_threshold_g2,
    idle_transition_time_s,
):
    print("Config update requested:")
    print("Inversion threshold: {:.2f} dps".format(inversion_threshold_dps))
    print("Actuated timeout: {:.2f} s".format(actuated_timeout_s))
    print("Idle variance threshold: {:.2f} G^2".format(idle_variance_threshold_g2))
    print("Idle transition time: {:.2f} s".format(idle_transition_time_s))
    request = "CFG:inv_thresh:{},act_time:{},idle_var:{:.2f},idle_time:{}".format(
        int(inversion_threshold_dps),
        int(actuated_timeout_s * 1000),
        idle_variance_threshold_g2,
        int(idle_transition_time_s * 1000),
    )
    send_tcp_data(request)


def update_telemetry(data):
    inv_str = data.split("INV:")[1].split(",ST:")[0]
    invs = inv_str.split(",")
    for inv in invs:
        g_update_inversion_speed(float(inv))
        time.sleep(1 / 1000)

    state_str = data.split(",ST:")[1].split(",VB:")[0]
    g_update_state(string_to_state[state_str])

    vbat_str = data.split(",VB:")[1].split(",inv_thresh")[0]
    g_update_vbatt(float(vbat_str) / 1000.0)

    inv_thresh_str = data.split("inv_thresh:")[1].split(",act_time")[0]
    act_time_str = data.split("act_time:")[1].split(",idle_var")[0]
    idle_var_str = data.split("idle_var:")[1].split(",idle_time")[0]
    idle_time_str = data.split("idle_time:")[1]
    g_update_config(
        float(inv_thresh_str),
        float(act_time_str) / 1000.0,
        float(idle_var_str),
        float(idle_time_str) / 1000.0,
    )


def user_thread():
    # init ESP32
    send_tcp_data(
        f"CFG:inv_thresh:{default_config['inv_thresh']},act_time:{default_config['act_time']},idle_var:{default_config['idle_var']},idle_time:{default_config['idle_time']}"
    )
    g_update_config(
        default_config["inv_thresh"],
        default_config["act_time"] / 1000.0,
        default_config["idle_var"],
        default_config["idle_time"] / 1000.0,
    )

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # Bind the socket to all interfaces
    server_address = ("", UDP_LISTEN_PORT)
    print(f"Starting UDP listen on port {UDP_LISTEN_PORT}")
    sock.bind(server_address)
    sock.settimeout(UDP_SOCKET_TIMEOUT_S)
    send_tcp_data(f"TELEMETRY_START:{UDP_LISTEN_PORT}")
    print("ESP32 Telemetry Started")

    while not g_gui_exited():
        print("\nWaiting to receive message...")
        data, address = sock.recvfrom(4096)
        print("Received {} bytes from {}".format(len(data), address))
        data = data.decode()
        print(data)
        update_telemetry(data)


if __name__ == "__main__":
    g_init_gui()  # must be called first in the main thread
    g_set_calibration_requested_callback(calibration_requested_callback)
    g_set_config_update_requested_callback(config_update_requested_callback)

    t = threading.Thread(target=user_thread)
    t.start()

    g_run_gui()  # must be called in the main thread after starting all other threads

    t.join()
