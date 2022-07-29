from tkinter import ttk
from connection import Connection
from remoteInfo import RemoteInfo


class ConnectingPage:
    def __init__(self, root):
        self.root = root
        self.main_frame = ttk.Frame(root, borderwidth=10, width=200, height=100)

        self.label = ttk.Label(self.main_frame, text='Connecting')
        self.label.grid(column=0, row=0)

        self.progress_bar = ttk.Progressbar(self.main_frame, orient='horizontal',
                                            mode='indeterminate', length=100)
        self.progress_bar.grid(column=0, row=1)

        self.connection = None
        self.next_page = None
        self.fallback_page = None

    def set_next_page(self, next_page):
        self.next_page = next_page

    def set_fallback_page(self, fallback_page):
        self.fallback_page = fallback_page

    def entry_point(self, **kwargs):
        self.main_frame.grid(column=0, row=0)
        remote: RemoteInfo = kwargs['remote']
        self.connection = Connection(remote)
        self.connection.connect()
        self.root.protocol("WM_DELETE_WINDOW", self.close_window_callback)
        self.poll_changes()

    def poll_changes(self):
        if self.connection.connection_failed.is_set():
            self.grid_remove()
            self.fallback_page.entry_point()

        elif self.connection.connected_event.is_set():
            self.grid_remove()
            self.next_page.entry_point(connection=self.connection)
        else:
            self.main_frame.after(50, self.poll_changes)

    def grid_remove(self):
        self.main_frame.grid_remove()
        self.progress_bar.stop()

    def close_window_callback(self):
        self.connection.close()
        self.root.destroy()
