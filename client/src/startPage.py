from tkinter import *
from tkinter import ttk
from os import path
import options
from remoteInfo import RemoteInfo


class StartPage:
    def __init__(self, root):
        self.next_page = None
        self.remotes_path = path.join(options.assets_path, 'remotes.dat')
        self.remotes = []
        self.single_edit = False

        self.main_frame = ttk.Frame(root, borderwidth=10, width=200, height=100)

        self.top_label = ttk.Label(self.main_frame, text="Saved connections:")
        self.top_label.grid(column=0, row=0)

        self.remotes_list_frame = ttk.Frame(self.main_frame)
        self.remotes_list_frame.grid(column=0, row=1)

        self.bottom_frame = ttk.Frame(self.main_frame)
        self.bottom_frame.grid(column=0, row=2)

        add_icon_path = path.join(options.assets_path, 'add_icon.gif')
        self.add_icon = PhotoImage(file=add_icon_path)
        self.add_button = ttk.Button(self.bottom_frame, image=self.add_icon, command=self.add_remote)

        self.edit_button = ttk.Button(self.bottom_frame, text='Edit', command=self.edit_remotes)

        self.discard_button = ttk.Button(self.bottom_frame, text='Discard', command=self.discard_changes)

        self.save_button = ttk.Button(self.bottom_frame, text='Save', command=self.save_changes)

    def set_next_page(self, next_page):
        self.next_page = next_page

    def entry_point(self):
        self.main_frame.grid(column=0, row=0)
        self.remotes = []
        self.load_from_file()

    def load_from_file(self):
        self.remotes_list_grid_remove()
        self.remotes = []
        for remote in RemoteInfo.load_from_file(self.remotes_path):
            self.add_remote(remote)

        self.remotes_list_grid_remove()
        for idx, remote in enumerate(self.remotes):
            remote[1].grid(column=0, row=idx)

        self.set_bottom_frame(initial=True)

    def remotes_list_grid_remove(self):
        for remote in self.remotes:
            remote[1].grid_forget()
            remote[2].grid_forget()

    def set_bottom_frame(self, initial: bool):
        self.add_button.grid_forget()
        self.edit_button.grid_forget()
        self.discard_button.grid_forget()
        self.save_button.grid_forget()

        if initial:
            self.add_button.grid(column=0, row=0)
            self.edit_button.grid(column=1, row=0)
        else:
            self.discard_button.grid(column=0, row=0)
            self.save_button.grid(column=1, row=0)

    def discard_changes(self):
        self.remotes_list_grid_remove()
        self.load_from_file()
        self.single_edit = False

    def save_changes(self):
        self.remotes_list_grid_remove()
        for idx, remote in enumerate(self.remotes):
            remote[1].grid(column=0, row=idx)
            remote[0].name = remote[3][0].get()
            remote[0].ip_address = [int(byte.get()) for byte in remote[3][1]]
            remote[0].port = int(remote[3][2].get())
        RemoteInfo.save_to_file([remote[0] for remote in self.remotes], self.remotes_path)
        self.set_bottom_frame(initial=True)
        self.single_edit = False

    def add_remote(self, remote=None):
        if remote is None:
            remote = RemoteInfo()
            self.single_edit = True

        name_var = StringVar(value=remote.name)
        ip_vars = [StringVar(value=byte) for byte in remote.ip_address]
        port_var = StringVar(value=remote.port)
        variables = (name_var, ip_vars, port_var)
        new_tuple = (remote, self.basic_frame(remote, variables),
                     self.edit_frame(remote, variables),
                     variables)
        new_tuple[2].grid(column=0, row=len(self.remotes))
        self.remotes.append(new_tuple)
        self.set_bottom_frame(initial=False)

    def edit_remotes(self):
        for idx, remote in enumerate(self.remotes):
            remote[2].grid(column=0, row=idx)
        self.set_bottom_frame(initial=False)

    def remove_remote(self, remote: RemoteInfo):
        self.remotes_list_grid_remove()
        self.remotes = [rem for rem in self.remotes if rem[0] is not remote]
        if self.single_edit:
            self.save_changes()
        else:
            for idx, remote in enumerate(self.remotes):
                remote[2].grid(column=0, row=idx)

    def basic_frame(self, remote: RemoteInfo, variables):
        frame = ttk.Frame(self.remotes_list_frame)

        name_entry = ttk.Label(frame, textvariable=variables[0])
        name_entry.grid(column=0, row=0)

        ip_labels = [ttk.Label(frame, textvariable=var) for var in variables[1]]
        for idx, entry in enumerate(ip_labels):
            entry.grid(column=idx * 2 + 2, row=0)

        port_entry = ttk.Label(frame, width=5, textvariable=variables[2])
        port_entry.grid(column=10, row=0)

        connect_button = ttk.Button(frame, text='connect',
                                    command=lambda: self.start_connection(remote))
        connect_button.grid(column=11, row=0)

        self.add_separators(frame)

        return frame

    def edit_frame(self, remote: RemoteInfo, variables):
        frame = ttk.Frame(self.remotes_list_frame)

        name_entry = ttk.Entry(frame, textvariable=variables[0])
        name_entry.grid(column=0, row=0)

        ip_entries = [ttk.Entry(frame, justify=RIGHT, width=3, textvariable=var) for var in variables[1]]
        for idx, entry in enumerate(ip_entries):
            entry.grid(column=idx * 2 + 2, row=0)

        port_entry = ttk.Entry(frame, justify=RIGHT, width=5, textvariable=variables[2])
        port_entry.grid(column=10, row=0, padx=(0, 15))

        self.add_separators(frame)

        up_button = ttk.Button(frame, text='Up')
        up_button.grid(column=11, row=0)

        down_button = ttk.Button(frame, text='Down')
        down_button.grid(column=12, row=0)

        delete_button = ttk.Button(frame, text='Delete', command=lambda: self.remove_remote(remote))
        delete_button.grid(column=13, row=0)

        return frame

    @staticmethod
    def add_separators(parent_frame):
        separator = ttk.Label(parent_frame, text=': ')
        separator.grid(column=1, row=0)

        ip_separators = [ttk.Label(parent_frame, text='.') for _ in range(3)]
        for idx, separator in enumerate(ip_separators):
            separator.grid(column=idx * 2 + 3, row=0)

        port_separator = ttk.Label(parent_frame, text=':')
        port_separator.grid(column=9, row=0)

    def start_connection(self, remote: RemoteInfo):
        self.main_frame.grid_remove()
        self.next_page.entry_point(remote=remote)
