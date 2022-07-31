from controlWidgets import *
import os
from os import path
import options
from connection import Connection
import pickle
from queue import Queue
from copy import deepcopy


class PageManager:
    def __init__(self, root, connection: Connection):
        self.main_frame = ttk.Frame(root)
        self.current_page = None
        self.widgets = []
        self.position_assigner = None
        self.current_id = None
        self.page_description_folder = path.join(options.assets_path, 'saved_pages')
        connection_desc = connection.remote_address[0].replace('.', '_') + '_' + str(connection.remote_address[1])
        self.connection_description_folder = path.join(self.page_description_folder, connection_desc)
        self.page_size = None
        self.event_queue = Queue()

    def grid(self):
        self.main_frame.grid(column=0, row=0)

    def grid_remove(self):
        self.main_frame.grid_remove()

    def change_page(self, page_id: int) -> bool:
        self.event_queue.queue.clear()
        page_description = self._load_page_description(page_id)
        if not page_description:
            return False

        for widget in self.widgets:
            widget.grid_remove()
        self.widgets = []

        self.event_queue.queue.clear()
        self.position_assigner = PositionAssigner(page_description)
        self.current_id = 0

        self.parse_widgets(page_description['widgets'])
        return True

    def parse_widgets(self, widgets: list):
        for widget in widgets:
            self.parse_widget(widget)

    def parse_widget(self, widget: dict):
        widget_class = widget_type_str[widget['type']]
        if widget_class is LabelElement:
            new_widget = widget_class(self.main_frame, -1, self.event_queue, widget)
        else:
            new_widget = widget_class(self.main_frame, self.current_id, self.event_queue, widget)
            self.current_id += 1

        self.widgets.append(new_widget)
        if 'position' in widget:
            self.position_assigner.set_position(widget['position'])
        new_widget.grid(**self.position_assigner.get_position())

    def update(self, values: bytes):
        for widget in self.widgets:
            if type(widget) is LabelElement:
                continue
            values = widget.update(values)

    def set_page_description(self, page_id: int, page_description: dict):
        if not path.isdir(self.page_description_folder):
            os.mkdir(self.page_description_folder)

        if not path.isdir(self.connection_description_folder):
            os.mkdir(self.connection_description_folder)

        file_path = self._file_path(page_id)
        with open(file_path, 'wb') as file:
            pickle.dump(page_description, file)

        self.change_page(page_id)

    def _load_page_description(self, page_id: int) -> dict:
        file_path = self._file_path(page_id)
        try:
            with open(file_path, 'rb') as file:
                return pickle.load(file)
        except FileNotFoundError:
            return dict()

    def _file_path(self, page_id) -> str:
        return path.join(self.connection_description_folder, str(page_id) + '.dat')


class PositionAssigner:
    def __init__(self, description: dict):
        if 'size' in description:
            self.height = description['size'][0]
            self.width = description['size'][1]
        else:
            self.height = len(description['widgets'])
            self.width = 1
        self.current_position = {'column': 0, 'row': 0}

    def set_position(self, position: list):
        self.current_position = {'column': position[1], 'row': position[0]}

    def get_position(self):
        temp = deepcopy(self.current_position)
        self.increment()
        return temp

    def increment(self):
        self.current_position['column'] += 1
        if self.current_position['column'] == self.width:
            self.current_position['row'] += 1
            self.current_position['column'] = 0
