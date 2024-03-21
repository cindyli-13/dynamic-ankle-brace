import sys, signal, time
import pyqtgraph as pg
from PyQt6.QtWidgets import QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout, QGridLayout, QLabel, QPushButton, QGraphicsDropShadowEffect
from PyQt6.QtGui import QFont
from PyQt6.QtCore import QTimer, pyqtSignal
from enum import Enum


class State(Enum):
    Calibrating = 1
    Idle = 2
    Active = 3
    Actuated = 4
    Unknown = 5


VBATT_100_PERCENT = 4.2
VBATT_0_PERCENT = 3.0
VBATT_GOOD_THRESHOLD = 3.4
VBATT_LOW_THRESHOLD = 3.2

g_window = None
g_app = None
g_app_exit = False


class MainWindow(QMainWindow):
    vbatt_signal = pyqtSignal(float)
    inversion_speed_signal = pyqtSignal(float)
    state_signal = pyqtSignal(State)

    def __init__(self):
        super(MainWindow, self).__init__()

        self.LABEL_FONT = QFont('Helvetica', 24, QFont.Weight.ExtraLight)
        self.VALUE_FONT = QFont('Helvetica', 24, QFont.Weight.Medium)
        self.LIVE_PLOTTER_MAX_SAMPLES = 100
        self.LABEL_STYLESHEET = '''
            background-color: #3B3B3B;
            border-radius: 12px;
            padding: 6px;
        '''

        self.setWindowTitle('Dynamic Ankle Support')
        # self.setStyleSheet('background-color: #2C6FBB') # blue
        self.setStyleSheet('background-color: #1B1B1B') # matte black

        self.vbatt_signal.connect(self.update_battery)
        self.inversion_speed_signal.connect(self.update_plot)
        self.state_signal.connect(self.update_state)

        layout_parent = QHBoxLayout()
        layout_left = QVBoxLayout()
        layout_right = QVBoxLayout()
        layout_state = QHBoxLayout()
        layout_button = QHBoxLayout()
        layout_config = QGridLayout()
        layout_battery = QGridLayout()

        state_label = QLabel()
        state_label.setText('State')
        state_label.setFont(self.LABEL_FONT)
        state_label.setFixedSize(100, 40)
        state_label.setStyleSheet(self.LABEL_STYLESHEET)

        self.state = QLabel()
        self.state.setFont(self.VALUE_FONT)

        layout_state.addWidget(state_label, stretch=0)
        layout_state.addWidget(self.state, stretch=0)

        self.calibrate_button = QPushButton('Calibrate')
        self.calibrate_button.setFont(self.LABEL_FONT)
        self.calibrate_button.setStyleSheet('''
            background-color: #6B6B6B;
            border-style: outset;
            border-width: 1px;
            border-color: #7B7B7B;
            border-radius: 12px;
            padding: 8px;
        ''')
        shadow = QGraphicsDropShadowEffect(self)
        shadow.setOffset(2, 3)
        shadow.setBlurRadius(10)
        self.calibrate_button.setGraphicsEffect(shadow)
        self.calibrate_button.clicked.connect(self.calibrate_button_clicked)

        spacer1 = QLabel()

        layout_button.addWidget(self.calibrate_button, stretch=0)
        layout_button.addWidget(spacer1, stretch=1)

        config_label = QLabel()
        config_label.setText('Config')
        config_label.setFont(self.LABEL_FONT)
        config_label.setFixedSize(100, 40)
        config_label.setStyleSheet(self.LABEL_STYLESHEET)

        inversion_threshold_label = QLabel()
        inversion_threshold_label.setText('Inversion Threshold')
        inversion_threshold_label.setFont(self.LABEL_FONT)
        inversion_threshold_label.setStyleSheet(self.LABEL_STYLESHEET)
        layout_config.addWidget(inversion_threshold_label, 0, 0)
        
        self.inversion_threshold = QLabel()
        self.inversion_threshold.setText('300 °/s')
        self.inversion_threshold.setFont(self.VALUE_FONT)
        layout_config.addWidget(self.inversion_threshold, 0, 1)
        
        actuated_timeout_label = QLabel()
        actuated_timeout_label.setText('Actuated Timeout')
        actuated_timeout_label.setFont(self.LABEL_FONT)
        actuated_timeout_label.setStyleSheet(self.LABEL_STYLESHEET)
        layout_config.addWidget(actuated_timeout_label, 1, 0)
        
        self.actuated_timeout = QLabel()
        self.actuated_timeout.setText('2.5 s')
        self.actuated_timeout.setFont(self.VALUE_FONT)
        layout_config.addWidget(self.actuated_timeout, 1, 1)
        
        idle_variance_threshold_label = QLabel()
        idle_variance_threshold_label.setText('Idle Variance Threshold')
        idle_variance_threshold_label.setFont(self.LABEL_FONT)
        idle_variance_threshold_label.setStyleSheet(self.LABEL_STYLESHEET)
        layout_config.addWidget(idle_variance_threshold_label, 2, 0)
        
        self.idle_variance_threshold = QLabel()
        self.idle_variance_threshold.setText('1.2 G^2')
        self.idle_variance_threshold.setFont(self.VALUE_FONT)
        layout_config.addWidget(self.idle_variance_threshold, 2, 1)
        
        idle_transition_time_label = QLabel()
        idle_transition_time_label.setText('Idle Transition Time')
        idle_transition_time_label.setFont(self.LABEL_FONT)
        idle_transition_time_label.setStyleSheet(self.LABEL_STYLESHEET)
        layout_config.addWidget(idle_transition_time_label, 3, 0)
        
        self.idle_transition_time = QLabel()
        self.idle_transition_time.setText('60 s')
        self.idle_transition_time.setFont(self.VALUE_FONT)
        layout_config.addWidget(self.idle_transition_time, 3, 1)

        spacer2 = QLabel()

        layout_left.addLayout(layout_state, stretch=1)
        layout_left.addLayout(layout_button, stretch=1)
        layout_left.addWidget(spacer2, stretch=1)
        layout_left.addWidget(config_label, stretch=1)
        layout_left.addLayout(layout_config)
        layout_left.setContentsMargins(0,40,0,10)

        self.live_plotter = pg.PlotWidget()
        self.live_plotter.setBackground(None)
        self.live_plotter.setTitle('Ankle Inversion Speed', family='Helvetica', color='white', size='20pt')
        self.live_plotter.setLabel('left', 'Inversion Speed (°/s)', family='Helvetica', size='20pt')
        self.live_plotter.setLabel('bottom', 'Time (s)', family='Helvetica', size='20pt')
        self.live_plotter.showGrid(x=True, y=True)
        self.live_plotter_pen = pg.mkPen(color='white')
        self.live_plotter_x = []
        self.live_plotter_y = []
        self.live_plotter_line = self.live_plotter.plot(self.live_plotter_x, self.live_plotter_y, pen=self.live_plotter_pen)

        battery_label = QLabel()
        battery_label.setText('Battery')
        battery_label.setFont(self.LABEL_FONT)
        battery_label.setFixedSize(120, 40)
        battery_label.setStyleSheet('''
            background-color: #3B3B3B;
            border-radius: 12px;
            padding: 5px;
        ''')

        self.battery_percentage = QLabel()
        self.battery_percentage.setFont(self.VALUE_FONT)

        vbatt_label = QLabel()
        vbatt_label.setText('VBATT')
        vbatt_label.setFont(self.LABEL_FONT)
        vbatt_label.setFixedSize(120, 40)
        vbatt_label.setStyleSheet('''
            background-color: #3B3B3B;
            border-radius: 12px;
            padding: 5px;
        ''')

        self.vbatt_value = QLabel()
        self.vbatt_value.setFont(self.VALUE_FONT)

        layout_battery.addWidget(battery_label, 0, 0)
        layout_battery.addWidget(self.battery_percentage, 0, 1)
        layout_battery.addWidget(vbatt_label, 1, 0)
        layout_battery.addWidget(self.vbatt_value, 1, 1)

        layout_right.addWidget(self.live_plotter)
        layout_right.addLayout(layout_battery)
        layout_right.setContentsMargins(80,0,0,0)

        layout_parent.addLayout(layout_left, stretch=1)
        layout_parent.addLayout(layout_right, stretch=2)
        layout_parent.setContentsMargins(40,40,40,40)
        layout_parent.setSpacing(60)

        widget = QWidget()
        widget.setLayout(layout_parent)
        self.setCentralWidget(widget)

        self.start_time = time.time()

        self.timer = QTimer()
        self.timer.setInterval(100)
        self.timer.timeout.connect(self.update)
        self.timer.start()

    def update_state(self, state):
        if state is State.Calibrating:
            self.state.setText('CALIBRATING')
            self.state.setStyleSheet('color: #E0B0FF')
        elif state is State.Idle:
            self.state.setText('IDLE')
            self.state.setStyleSheet('color: #89CFF0')
        elif state is State.Active:
            self.state.setText('ACTIVE')
            self.state.setStyleSheet('color: #39E75F')
        elif state is State.Actuated:
            self.state.setText('ACTUATED')
            self.state.setStyleSheet('color: #EE4B2B')
        else:
            self.state.setText('')

    def update_plot(self, inversion_speed):
        self.live_plotter_x.append(time.time() - self.start_time)
        self.live_plotter_y.append(inversion_speed)
        if len(self.live_plotter_x) > self.LIVE_PLOTTER_MAX_SAMPLES:
            self.live_plotter_x = self.live_plotter_x[1:]
            self.live_plotter_y = self.live_plotter_y[1:]
        self.live_plotter_line.setData(self.live_plotter_x, self.live_plotter_y)

    def update_battery(self, vbatt):
        percentage = (vbatt - VBATT_0_PERCENT) / (VBATT_100_PERCENT - VBATT_0_PERCENT) * 100.0
        percentage = min(100.0, max(0.0, percentage)) # clamp between 0% and 100%
        self.battery_percentage.setText('{}%'.format(int(percentage)))
        self.vbatt_value.setText('{:.2f} V'.format(vbatt))
        if vbatt > VBATT_GOOD_THRESHOLD:
            self.battery_percentage.setStyleSheet('color: #39E75F')
            self.vbatt_value.setStyleSheet('color: #39E75F')
        elif vbatt > VBATT_LOW_THRESHOLD:
            self.battery_percentage.setStyleSheet('color: #FFBF00')
            self.vbatt_value.setStyleSheet('color: #FFBF00')
        else:
            self.battery_percentage.setStyleSheet('color: #EE4B2B')
            self.vbatt_value.setStyleSheet('color: #EE4B2B')

    def calibrate_button_clicked(self):
        print('calibrate')


def g_update_inversion_speed(inversion_speed: float):
    global g_window
    g_window.inversion_speed_signal.emit(inversion_speed)


def g_update_state(state: State):
    global g_window
    g_window.state_signal.emit(state)


def g_update_vbatt(vbatt: float):
    global g_window
    g_window.vbatt_signal.emit(vbatt)


def g_init_gui():
    global g_app
    global g_window
    signal.signal(signal.SIGINT, signal.SIG_DFL)
    g_app = QApplication(sys.argv)
    g_window = MainWindow()
    g_window.showFullScreen()


def g_run_gui():
    global g_app
    global g_app_exit
    g_app.exec()
    g_app_exit = True


def g_gui_exited():
    global g_app_exit
    return g_app_exit
