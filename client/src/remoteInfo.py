import pickle


class RemoteInfo:
    idx = 0

    def __init__(self):
        self.name = f'Remote{RemoteInfo.idx}'
        RemoteInfo.idx += 1
        self.ip_address = [0, 0, 0, 0]
        self.port = 0

    @classmethod
    def load_from_file(cls, file_path: str) -> list:
        try:
            with open(file_path, 'rb') as remotes_file:
                return pickle.load(remotes_file)
        except FileNotFoundError:
            return []

    @classmethod
    def save_to_file(cls, remotes, file_path: str):
        with open(file_path, 'wb') as remotes_file:
            pickle.dump(remotes, remotes_file)
