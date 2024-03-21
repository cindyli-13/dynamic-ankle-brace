import threading, random
from gui import *


def user_thread():
        counter = 0
        vbatt = 4.2
        states = [State.Calibrating, State.Idle, State.Active, State.Actuated]
        while not g_gui_exited():
            counter += 1
            g_update_inversion_speed(random.random() * 100.0)
            if counter >= 10:
                g_update_state(random.choice(states))
                g_update_vbatt(vbatt)
                counter = 1
                vbatt -= 0.01
                if vbatt <= 0.0:
                    vbatt = 4.2
            time.sleep(0.1)


if __name__ == '__main__':
    g_init_gui()

    t = threading.Thread(target=user_thread)
    t.start()

    g_run_gui()

    t.join()
