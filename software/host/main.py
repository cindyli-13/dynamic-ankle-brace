import threading, random
from gui import *


def user_thread():
        counter = 0
        vbatt = 4.2
        states = [State.Calibrating, State.Idle, State.Active, State.Actuated]

        g_update_config(300, 2.5, 1.2, 60)

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

            if g_is_calibration_requested():
                g_clear_calibration_request()
                print('Calibration requested')

            new_config_params = g_maybe_get_new_config_params()
            if new_config_params is not None:
                print('New config requested:', new_config_params)
                g_update_config(new_config_params[0], new_config_params[1], new_config_params[2], new_config_params[3])

            time.sleep(0.1)


if __name__ == '__main__':
    g_init_gui() # must be called first in the main thread

    t = threading.Thread(target=user_thread)
    t.start()

    g_run_gui() # must be called in the main thread after starting all other threads

    t.join()
