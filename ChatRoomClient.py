from PyQt5 import QtCore, QtGui, QtWidgets
import sys 
import os
import webbrowser
from ctypes import cdll
import ctypes
from time import sleep
from threading import Thread, Lock
from ChatRoom_GUI import Ui_MainWindow

lib = cdll.LoadLibrary('./client.so')

class Worker(QtCore.QObject):
    listened = QtCore.pyqtSignal()

    def __init__(self):
        QtCore.QObject.__init__(self)

    text = ''
    signal = 0

    def listen(self):
        lib.recv_msg.restype = ctypes.c_char_p
        while self.signal == 0:
            tmp_text = lib.recv_msg()[:-2].decode("utf-8")
            if not tmp_text == '':
                self.text = tmp_text
                self.listened.emit()

    def run(self):
        threadListen = Thread(target=self.listen, args=({}))
        threadListen.start()

class ChatRoomClient(QtWidgets.QMainWindow):
    def __init__(self):
        super(ChatRoomClient, self).__init__()
        self.chatRoomClient = Ui_MainWindow()
        self.chatRoomClient.setupUi(self)

        self.ipAddress = ''
        self.port = ''
        self.roomListMessage = ''
        self.name = ''
        self.chatRoomName = ''
        self.roomName = []
        self.isChat = 0
        self.rcvMessage = ''

        self.work = Worker()
        self.work.listened.connect(self.receiveMessage)
        
        self.chatRoomClient.mainTabWidget.setTabEnabled(1,False)
        self.chatRoomClient.buttonJoinRoom.setEnabled(False)
        self.chatRoomClient.buttonCreateRoom.setEnabled(False)
        self.chatRoomClient.lineEditUserName.setEnabled(False)
        self.chatRoomClient.lineEditChatRoomName.setEnabled(False)
        self.chatRoomClient.lineEditIpAddress.setValidator(QtGui.QRegExpValidator(QtCore.QRegExp("^\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}$")))
        self.chatRoomClient.lineEditPort.setValidator(QtGui.QIntValidator())
        self.chatRoomClient.lineEditChatRoomName.setValidator(QtGui.QIntValidator())
        
        self.chatRoomClient.lineEditUserName.textChanged.connect(self.chatRoomClient.labelNumberUsingApp.clear)
        self.chatRoomClient.buttonConnect.clicked.connect(self.connectServerClicked)
        self.chatRoomClient.buttonCreateRoom.clicked.connect(self.createRoom)
        self.chatRoomClient.buttonJoinRoom.clicked.connect(self.jointRoom)
        self.chatRoomClient.buttonSendMessage.clicked.connect(self.sendMessage)
        self.chatRoomClient.buttonExitRoom.clicked.connect(self.exitRoom)
        self.chatRoomClient.buttonHelp.clicked.connect(self.openManual)

    def keyPressEvent(self, event):
        if event.key() == QtCore.Qt.Key_Return:
            if self.isChat:
                self.sendMessage()

    def connectServerClicked(self):
        self.ipAddress = self.chatRoomClient.lineEditIpAddress.text()
        self.port = self.chatRoomClient.lineEditPort.text()
        threadConnect = Thread(target=self.connectServer, args=({}))
        threadConnect.start()
        lib.recv_room_list.restype = ctypes.c_char_p
        self.roomListMessage = lib.recv_room_list()[:-1].decode("utf-8")
        self.setRoomList()
        self.chatRoomClient.buttonCreateRoom.setEnabled(True)
        self.chatRoomClient.lineEditUserName.setEnabled(True)
        self.chatRoomClient.lineEditChatRoomName.setEnabled(True)
        
    def connectServer(self):
        b_ipAddress = self.ipAddress.encode('utf-8')
        port = int(self.port)
        lib.connectServer.argtypes = [ctypes.c_char_p, ctypes.c_int]
        lib.connectServer(b_ipAddress, port)

    def setRoomList(self):
        if len(self.roomListMessage.split('Current existing rooms: \n'))>1:
            roomMessageDetail = self.roomListMessage.split('Current existing rooms: \n')[1]
            roomlist = roomMessageDetail.split('\n')
            for room in roomlist:
                if not room == '':
                    self.roomName.append(room)
            if len(self.roomName) > 0:
                self.radioButtonRooms = [QtWidgets.QRadioButton(self.chatRoomClient.scrollAreaWidgetContents_2) for i in range(len(self.roomName))]
                for i in range(len(self.roomName)):
                    self.radioButtonRooms[i].setText(self.roomName[i])
                    self.chatRoomClient.gridLayout_6.addWidget(self.radioButtonRooms[i], i, 0, 1, 1)
                self.roomListMessage = ''    
                self.chatRoomClient.buttonJoinRoom.setEnabled(True)
        
    def createRoom(self):
        self.name = self.chatRoomClient.lineEditUserName.text()
        if len(self.name) < 2 or len(self.name) > 32:
            self.chatRoomClient.labelNumberUsingApp.setText("<p style=\" color:#ff0000;\">User name need to be from 2 to 32 symbols</p>")
        else:
            self.chatRoomName = self.chatRoomClient.lineEditChatRoomName.text()
            if self.chatRoomName == '':
                self.chatRoomClient.labelNumberUsingApp.setText("<p style=\" color:#ff0000;\">No room name chosen</p>")
            else:
                self.handleRoom(self.name, '1', self.chatRoomName)
                self.chatRoomClient.mainTabWidget.setTabEnabled(1,True)
                self.chatRoomClient.mainTabWidget.setCurrentIndex(1)
                self.chatRoomClient.mainTabWidget.setTabEnabled(0,False)
                self.chatRoomClient.labelChatRoomName.setText('Chat Room #' + self.chatRoomName)
                self.isChat = 1
                self.work.run()

    def jointRoom(self):
        self.name = self.chatRoomClient.lineEditUserName.text()
        if len(self.name) < 2 or len(self.name) > 32:
            self.chatRoomClient.labelNumberUsingApp.setText("<p style=\" color:#ff0000;\">User name need to be from 2 to 32 symbols</p>")
        else:
            for i in range(len(self.radioButtonRooms)):
                if self.radioButtonRooms[i].isChecked():
                    title = self.radioButtonRooms[i].text()
            if title == '' or not title:
                self.chatRoomClient.labelNumberUsingApp.setText("<p style=\" color:#ff0000;\">No room name chosen</p>")
            else:
                self.chatRoomName = title.split('Room ID: ')[1].split(' hosted by Bili')[0]
                self.handleRoom(self.name, '2', self.chatRoomName)
                self.chatRoomClient.mainTabWidget.setTabEnabled(1,True)
                self.chatRoomClient.mainTabWidget.setCurrentIndex(1)
                self.chatRoomClient.mainTabWidget.setTabEnabled(0,False)
                self.chatRoomClient.labelChatRoomName.setText('Chat Room #' + self.chatRoomName)
                self.isChat = 1
                self.work.run()

    def handleRoom(self, name, opt, roomName):
        self.chatRoomClient.labelChatContent.clear()
        b_name = name.encode('utf-8')
        b_opt = opt.encode('utf-8')
        b_roomName = roomName.encode('utf-8')
        lib.username_handler.argtypes = [ctypes.c_char_p]
        lib.option_handler.argtypes = [ctypes.c_char_p]
        lib.room_handler.argtypes = [ctypes.c_char_p]
        lib.username_handler(b_name)
        lib.option_handler(b_opt)
        lib.room_handler(b_roomName)

    def sendMessage(self):
        oldText = self.chatRoomClient.labelChatContent.text()
        text = self.chatRoomClient.lineEditMessage.text()
        if not text == '':
            b_text = text.encode('utf-8')
            lib.room_handler.argtypes = [ctypes.c_char_p]
            lib.send_msg(b_text)
            self.chatRoomClient.lineEditMessage.clear()
            newText = oldText + '<div align="right"><font color="#0000FF">Me: '+ text +'</font></div>'
            self.chatRoomClient.labelChatContent.setText(newText)
        

    def receiveMessage(self):
        got_text = self.work.text
        oldText = self.chatRoomClient.labelChatContent.text()
        newText = oldText + '<div>'+ got_text +'</font></div>'
        self.chatRoomClient.labelChatContent.setText(newText)

    def openManual(self):
        if os.path.exists('Help and Manual.pdf'):
            webbrowser.open_new('Help and Manual.pdf')
        else:
            pass
    
    def exitRoom(self):
        self.work.signal = 1
        self.isChat = 0
        lib.catch_ctrl_c_and_exit()
        exit()
        # self.chatRoomClient.mainTabWidget.setTabEnabled(0,True)
        # self.chatRoomClient.mainTabWidget.setCurrentIndex(0)
        # self.chatRoomClient.mainTabWidget.setTabEnabled(1,False)
        # for i in range(len(self.roomName)):
        #     self.chatRoomClient.gridLayout_6.removeWidget(self.radioButtonRooms[i])
        #     self.radioButtonRooms[i].deleteLater()
        #     self.radioButtonRooms[i] = None
        #     # self.radioButtonRooms[i].setParent(None)
        # self.roomName = []
        # self.chatRoomClient.lineEditUserName.clear()
        # self.chatRoomClient.lineEditUserName.setEnabled(False)
        # self.chatRoomClient.lineEditChatRoomName.clear()
        # self.chatRoomClient.lineEditChatRoomName.setEnabled(False)
        # self.chatRoomClient.buttonCreateRoom.setEnabled(False)
        # self.chatRoomClient.buttonJoinRoom.setEnabled(False)

def run():
    sleep(1)
    app = QtWidgets.QApplication(sys.argv)
    client = ChatRoomClient()
    client.show()
    sys.exit(app.exec_())

if __name__ == "__main__":
    run()  
