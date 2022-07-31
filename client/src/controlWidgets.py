from tkinter import *
from tkinter import ttk
from queue import Queue


class BaseElement:
    def __init__(self, root, element_id: int, event_queue: Queue):
        self.root = root
        self.main_frame = ttk.Frame(root)
        self.element_id = element_id
        self.change_callback_queue = event_queue
        self.enabled = False

    def grid(self, **kwargs):
        self.main_frame.grid(**kwargs)

    def grid_remove(self):
        self.main_frame.grid_remove()


class ButtonElement(BaseElement):
    def __init__(self, root, element_id: int, event_queue: Queue, description: dict):
        super().__init__(root, element_id, event_queue)
        self.text: str = description['text']
        self.text_disabled = description['text_disabled'] if 'text_disabled' in description else ""
        self.button = ttk.Button(self.main_frame, text=self.text)
        self.button.bind('<Button-1>', self.press_callback)
        self.button.bind('<ButtonRelease-1>', self.release_callback)
        self.button.grid(column=0, row=0)
        self.pressed = False

    def update(self, value: bytes):
        self.pressed = bool(int.from_bytes(value[:4], byteorder='little'))
        self.enabled = bool(value[4])

        self.button.state([
            'pressed' if self.pressed else '!pressed',
            '!disabled' if self.enabled else 'disabled'
        ])
        return value[5:]

    def press_callback(self, _):
        self.change_callback_queue.put([self.element_id, 1])
        print("Pressed:", self.element_id)

    def release_callback(self, _):
        self.change_callback_queue.put([self.element_id, 0])
        print("Released:", self.element_id)


class LabelElement(BaseElement):
    def __init__(self, root, element_id: int, event_queue: Queue, description: dict):
        super().__init__(root, element_id, event_queue)
        self.label = ttk.Label(self.main_frame, text=description['text'])
        self.label.grid(column=0, row=0)


class EntryElement(BaseElement):
    def __init__(self, root, element_id: int, event_queue: Queue, description: dict):
        super().__init__(root, element_id, event_queue)
        self.text: str = ''
        if 'text' in description:
            self.text = description['text']
        self.label = ttk.Label(self.main_frame, text=self.text)
        self.tk_value = StringVar()
        self.entry = ttk.Entry(self.main_frame, textvariable=self.tk_value)
        self.entry.bind('<FocusIn>', self.focus_in_callback)
        self.entry.bind('<FocusOut>', self.focus_out_callback)
        self.entry.bind('<Return>', self.return_press_callback)
        self.label.grid(column=0, row=0)
        self.entry.grid(column=1, row=0)
        self.has_focus = False

    def return_press_callback(self, _):
        self.main_frame.focus_set()

    def focus_in_callback(self, _):
        self.has_focus = True

    def focus_out_callback(self, _):
        self.has_focus = False
        if self.enabled:
            self.change_callback_queue.put([self.element_id, self.tk_value.get()])

    def update(self, value: bytes):
        string_len = value.find(0)
        if not self.has_focus:
            self.tk_value.set(value[:string_len].decode())
        self.enabled = bool(value[string_len+1])
        return value[string_len+2:]


class ValueElement(LabelElement):
    pass


class SwitchElement(BaseElement):
    def __init__(self, root, element_id: int, event_queue: Queue, description: dict):
        super().__init__(root, element_id, event_queue)
        self.value = IntVar(value=0)
        self.texts = description['text'].split(',')
        self.buttons = []
        for idx, text in enumerate(self.texts):
            rd = Radiobutton(self.main_frame, text=text, variable=self.value,
                             value=idx, command=self.change_callback,
                             indicatoron=False, width=6)
            rd.grid(column=idx+1, row=0)
            self.buttons.append(rd)

    def change_callback(self):
        self.change_callback_queue.put([self.element_id, self.value.get()])

    def update(self, value: bytes):
        self.value.set(int.from_bytes(value[:4], byteorder='little'))
        self.enabled = bool(value[4])

        return value[5:]


widget_type_str = {
    "button": ButtonElement,
    "label": LabelElement,
    "entry": EntryElement,
    "value": ValueElement,
    "switch": SwitchElement
}
