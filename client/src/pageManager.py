from controlWidgets import *
import os
from os import path
import options
from connection import Connection
import pickle
from queue import Queue


class PageManager:
    def __init__(self, root, connection: Connection):
        self.main_frame = ttk.Frame(root)
        self.current_page = None
        self.widgets = []
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

        if 'size' not in page_description:
            self.page_size = [len(page_description['widgets']), 1]
        else:
            self.page_size = page_description['size']

        self.parse_widgets(page_description['widgets'])
        return True

    def parse_widgets(self, widgets: list):
        for idx, widget in enumerate(widgets):
            self.parse_widget(idx, widget)

    def parse_widget(self, idx: int, widget: dict):
        widget_class = controlWidgets.widget_type_str[widget['type']]
        new_widget = widget_class(self.main_frame, idx, self.event_queue, widget)
        self.widgets.append(new_widget)
        page_width = self.page_size[1]
        new_widget.grid(column=idx % page_width,
                        row=idx // page_width)

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
