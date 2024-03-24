import threading, random
from gui import *


def calibration_requested_callback():
    print('Calibration requested')


def config_update_requested_callback(inversion_threshold_dps, actuated_timeout_s, idle_variance_threshold_g2, idle_transition_time_s):
    print('Config update requested:')
    print('Inversion threshold: {:.2f} dps'.format(inversion_threshold_dps))
    print('Actuated timeout: {:.2f} s'.format(actuated_timeout_s))
    print('Idle variance threshold: {:.2f} G^2'.format(idle_variance_threshold_g2))
    print('Idle transition time: {:.2f} s'.format(idle_transition_time_s))
    g_update_config(inversion_threshold_dps, actuated_timeout_s, idle_variance_threshold_g2, idle_transition_time_s)


def user_thread():
        counter = 0
        vbatt = 4.2
        states = [State.Calibrating, State.Idle, State.Active, State.Actuated]

        g_update_config(300, 2.5, 1.2, 60)

        g_set_calibration_requested_callback(calibration_requested_callback)
        g_set_config_update_requested_callback(config_update_requested_callback)

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
    g_init_gui() # must be called first in the main thread

    t = threading.Thread(target=user_thread)
    t.start()

    g_run_gui() # must be called in the main thread after starting all other threads

    t.join()
