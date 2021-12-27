from socket import AF_INET, socket, SOCK_STREAM
from threading import Thread
import time
import tkinter
from tkinter.constants import END


# This function must be threaded.
def receive_msg():
    global number_of_rooms
    global rooms_list
    global top
    while True:
        try:
            msg = client_socket.recv(BUFFER_SIZE).decode("utf8")
            if (msg == '#close'):
                top.destroy()
                break
            text = msg.partition(" ")
            if (text[0] == "newroom"):
                if (text[2]!=number_of_rooms):
                    print("added room " + text[2])
                    number_of_rooms = number_of_rooms + 1
                    chatRoomSelected.set('')
                    chat_rooms['menu'].delete(0, 'end')
                    rooms_list.append("Chat Room " + str(number_of_rooms))
                    for r in rooms_list:
                        chat_rooms['menu'].add_command(label=r, command=tkinter._setit(chatRoomSelected, r))
                    chatRoomSelected.set("List Of Chat Rooms")
            else:
                msg_list.insert(tkinter.END, msg)
                msg_list.see(tkinter.END)
        except OSError:
            break
    client_socket.close()

def send_msg(event=None): 
    msg = my_msg.get()
    my_msg.set("")      # Clears text field.
    global current_room
    client_socket.send(bytes(my_username.get() + ": " + msg, "utf8"))

def add_room():
    global number_of_rooms
    global rooms_list

    number_of_rooms = number_of_rooms + 1
    
    chatRoomSelected.set('')
    chat_rooms['menu'].delete(0, 'end')
    rooms_list.append("Chat Room " + str(number_of_rooms))
    for r in rooms_list:
        chat_rooms['menu'].add_command(label=r, command=tkinter._setit(chatRoomSelected, r))
    chatRoomSelected.set("List Of Chat Rooms")
    current_room = number_of_rooms;
    client_socket.send(bytes("#", "utf8"))
    msg_list.delete(0, tkinter.END)
    msg_list.insert(tkinter.END, "You are now in room " + str(current_room))
    msg_list.see(tkinter.END)


def change_room():
    global current_room
    current_room = ((chatRoomSelected.get()).split(' '))[2]
    client_socket.send(bytes("/" + current_room, "utf8"))
    msg_list.delete(0, tkinter.END)
    msg_list.insert(tkinter.END, "You are now in room " + str(current_room))
    msg_list.see(tkinter.END)


def shutdown():
    global flag
    client_socket.send(bytes(my_username.get() + " has closed OS Messenger App!", "utf8"))
    time.sleep(1)
    client_socket.send(bytes("shutdown", "utf8")) 

# Global variables.
number_of_rooms = 0
current_room = 0
flag = True

top = tkinter.Tk()
top.title("OS Messenger App")

messages_frame = tkinter.Frame(top)
my_msg = tkinter.StringVar()  # For the messages to be sent.
my_msg.set("")
my_username = tkinter.StringVar()
my_username.set("")

scrollbar = tkinter.Scrollbar(messages_frame)  # To see through previous messages.
# Messages container.
msg_list = tkinter.Listbox(messages_frame, height=30, width=100, yscrollcommand=scrollbar.set)
scrollbar.pack(side=tkinter.RIGHT, fill=tkinter.Y)
msg_list.pack(side=tkinter.LEFT, fill=tkinter.BOTH)
msg_list.pack()
messages_frame.pack()

username_label = tkinter.Label(top, text="Enter username: ")
username_label.pack()
username_field = tkinter.Entry(top, textvariable=my_username)
username_field.pack()

message_label = tkinter.Label(top, text="Enter message: ")
message_label.pack()
entry_field = tkinter.Entry(top, textvariable=my_msg, width=50)
entry_field.bind("<Return>", send_msg)
entry_field.pack()
send_button = tkinter.Button(top, text="Send", command=send_msg)
send_button.pack()


top.protocol("WM_DELETE_WINDOW", shutdown)

# Socket
HOST = "192.168.56.106"
PORT = 3000 #GCP port and IP
BUFFER_SIZE = 1024
ADDR = (HOST, PORT)

client_socket = socket(AF_INET, SOCK_STREAM)
client_socket.connect(ADDR)

# Server response of number of rooms available and generate drop down list.
first_msg = client_socket.recv(BUFFER_SIZE).decode("utf8")
number_of_rooms = int(first_msg)
chatRoomSelected = tkinter.StringVar(top)
chatRoomSelected.set("List Of Chat Rooms")
rooms_list = []
for i in range(number_of_rooms):
    rooms_list.append("Chat Room " + str(i + 1))

chat_rooms = tkinter.OptionMenu(top, chatRoomSelected, *rooms_list)
chat_rooms.pack()
change_button = tkinter.Button(top, text="Change Room", command=change_room)
change_button.pack()
add_room_button = tkinter.Button(top, text="Add Room", command=add_room)
add_room_button.pack()
exit_button = tkinter.Button(top, text="Exit", command=shutdown)
exit_button.pack()

receive_thread = Thread(target=receive_msg)
receive_thread.start()
top.resizable(width=False, height=False)    
tkinter.mainloop()

exit()