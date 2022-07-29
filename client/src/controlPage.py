from tkinter import ttk
from pageManager import PageManager


class ControlPage:
    def __init__(self, root):
        self.main_frame = ttk.Frame(root, borderwidth=10, width=200, height=100)

        self.init_frame = ttk.Frame(self.main_frame)
        self.connected_label = ttk.Label(self.init_frame, text='Connected')
        self.connected_label.grid(column=0, row=0)
        self.waiting_label = ttk.Label(self.init_frame, text='Waiting for data...')
        self.waiting_label.grid(column=0, row=1)

        self.page_manager = None

        self.connection = None
        self.fallback_page = None
        self.version = None
        self.requested_page_id = None

    def set_fallback_page(self, fallback_page):
        self.fallback_page = fallback_page

    def entry_point(self, **kwargs):
        self.connection = kwargs['connection']
        self.page_manager = PageManager(self.main_frame, self.connection)
        self.set_up_page()
        self.poll_changes()

    def set_up_page(self):
        self.main_frame.grid(column=0, row=0)
        self.init_frame.grid(column=0, row=0)

    def grid_remove(self):
        self.main_frame.grid_remove()

    def poll_changes(self):
        if self.connection.connection_failed.is_set():
            self.grid_remove()
            self.fallback_page.entry_point()

        elif self.connection.closed_event.is_set():
            self.grid_remove()
            self.fallback_page.entry_point()

        else:
            self.process_messages()
            self.main_frame.after(50, self.poll_changes)

    def process_messages(self):
        if self.connection.receive_queue.empty():
            return

        msg: dict = self.connection.receive_queue.get()
        response = {"CMD": "POLL"}

        if 'ERR' in msg:
            print(msg['ERR'])

        if 'VERSION' in msg:
            self.version = msg['VERSION']
            self.init_frame.grid_remove()
            self.page_manager.grid()

        if self.requested_page_id is not None:
            self.page_manager.set_page_description(self.requested_page_id, msg)
            self.requested_page_id = None

        if 'PAGE' in msg:
            if not self.page_manager.change_page(msg['PAGE']):
                self.requested_page_id = msg['PAGE']
                response = {"CMD": "GET", "VAL": {"PAGE": self.requested_page_id}}

        if 'VAL' in msg:
            self.page_manager.update(self.connection.receive_queue.get())

        if not self.page_manager.event_queue.empty() and response["CMD"] == 'POLL':
            event = self.page_manager.event_queue.get()
            response = {"CMD": "SET", "VAL": event}

        self.connection.send_queue.put(response)
