
import time
import subprocess

cmd = ['backdrop','-l', '2', '-x', '200', '-y' '500', '-w', '60', '-h', '78']

backdrop_process = subprocess.Popen(cmd, stdin=subprocess.PIPE)

time.sleep(4)

backdrop_process.stdin.write('400,100,400,300\n'.encode())
backdrop_process.stdin.flush()

time.sleep(4)

backdrop_process.terminate()
