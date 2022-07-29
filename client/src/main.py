from tkinter import *
from startPage import StartPage
from connectingPage import ConnectingPage
from controlPage import ControlPage

if __name__ == "__main__":
    root = Tk()
    start_page = StartPage(root)
    connecting_page = ConnectingPage(root)
    control_page = ControlPage(root)

    start_page.set_next_page(connecting_page)
    connecting_page.set_next_page(control_page)

    connecting_page.set_fallback_page(start_page)
    control_page.set_fallback_page(start_page)

    start_page.entry_point()
    root.mainloop()
