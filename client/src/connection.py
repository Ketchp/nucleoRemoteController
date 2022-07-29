import socket
import json
from threading import Thread, Event
from queue import Queue
from remoteInfo import RemoteInfo
import re
import time


class Connection:
    def __init__(self, remote: RemoteInfo):
        ip_address_string = '.'.join([str(byte) for byte in remote.ip_address])
        self.remote_address = (ip_address_string, remote.port)

        self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

        self.communication_thread = Thread(target=self.start_connection)

        # communication queues/mutexes for multithreading
        self.receive_queue = Queue()
        self.send_queue = Queue()
        self.kill_event = Event()
        self.connected_event = Event()
        self.connection_failed = Event()
        self.closed_event = Event()

    def connect(self):
        self.communication_thread.start()

    def close(self):
        self.kill_event.set()
        self.communication_thread.join()

    def start_connection(self):
        try:
            self.socket.settimeout(10)
            self.socket.connect(self.remote_address)
            self.socket.settimeout(5)
            self.connected_event.set()
        except (OSError, socket.timeout):
            self.connection_failed.set()
            return

        self.mainloop()

    def mainloop(self):
        while not (self.kill_event.is_set() or
                   self.closed_event.is_set() or
                   self.connection_failed.is_set()):
            self.communication_cycle()

        self.socket.close()

    def communication_cycle(self):
        try:
            msg = self.receive_with_prefix()
        except socket.timeout:
            self.connection_failed.set()
            return

        if not msg:
            self.closed_event.set()
            return

        try:
            received = json.loads(msg.decode())
            self.receive_queue.put(received)
        except json.JSONDecodeError as err:
            err_msg = err.args[0]
            p = re.compile("Extra data: line \\d+ column \\d+ \\(char (\\d+)\\)")
            m = p.match(err_msg)
            if m is None:
                self.connection_failed.set()
                return

            parse_len = int(m.group(1))
            try:
                received = json.loads(msg.decode()[:parse_len])
                self.receive_queue.put(received)
                self.receive_queue.put(msg[parse_len:])
            except json.JSONDecodeError:
                self.connection_failed.set()
                return

        response = self.send_queue.get()
        print('sending: ', response)
        if response is None:
            self.kill_event.set()
            return

        self.socket.send(json.dumps(response).encode())
        time.sleep(1)

    def receive_with_prefix(self) -> bytes:
        prefix = self.socket.recv(4)

        msg_len = int.from_bytes(prefix, byteorder='big')

        msg = self.socket.recv(msg_len)

        print('received:', msg)
        return msg
