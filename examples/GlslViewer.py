from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

# http://eyalarubas.com/python-subproc-nonblock.html
from subprocess import Popen, PIPE
import time
from fcntl import fcntl
from fcntl import F_GETFL
from fcntl import F_SETFL
from os import O_NONBLOCK
from os import read


class GlslViewer:
    COMMAND = 'glslViewer'
    process = None
    wait_time = 0.0001
    elapsed_time = 0.0
    delta = 0.0
    fps = 0.0
    cmd = ''
    options = {}

    def __init__(self, filename, options=None):
        if options is None:
            self.options = {}
        else:
            self.options = options

        # compose and excecute a glslViewer command
        self.cmd = [self.COMMAND]
        self.cmd.append(filename)

        if 'vertex' in options:
            self.cmd.append(options['vertex'])

        if 'size' in options:
            self.cmd.append('-w')
            self.cmd.append(str(options['size']))
            self.cmd.append('-h')
            self.cmd.append(str(options['size']))

        if 'headless' in options:
            self.cmd.append('--headless')

        if 'output' in options:
            self.cmd.append('-o')
            self.cmd.append(options['output'])

        if 'defines' in options:
            for define in options['defines']:
                self.cmd.append('-D' + str(define))

        if 'include_folders' in options:
            for folder in options['include_folders']:
                self.cmd.append('-I' + str(folder))

        if 'textures' in options:
            for tex_name in options['textures'].keys():
                self.cmd.append('-' + tex_name)
                self.cmd.append(options['textures'][tex_name])

        if 'textures_vflipped' in options:
            self.cmd.append('-vFlip')
            for tex_name in options['textures_vflipped'].keys():
                self.cmd.append('-' + tex_name)
                self.cmd.append(options['textures_vflipped'][tex_name])

    def start(self):
        if self.isRunning():
            return False
        # print('EXE ',  ' '.join(self.cmd))
        self.process = Popen(self.cmd,
                             stdin=PIPE, stdout=PIPE, stderr=PIPE,
                             shell=False)
        # get current self.process.stdout flags
        flags = fcntl(self.process.stdout, F_GETFL)
        fcntl(self.process.stdout, F_SETFL, flags | O_NONBLOCK)
        return True

    def stop(self):
        if not self.isRunning():
            return False

        self.write('exit')
        while not self.isFinish():
            time.sleep(1)
        self.process = None
        return True

    def isRunning(self):
        return self.process is not None

    def isFinish(self):
        if not self.isRunning():
            return False

        return self.process.poll() is not None

    def read(self):
        if not self.isRunning():
            return False
        while True:
            try:
                return read(self.process.stdout.fileno(), 1024).rstrip()
            except OSError:
                return 'none'

    def write(self, cmd):
        if not self.isRunning():
            return False
        self.process.stdin.write(cmd + '\n')
        time.sleep(self.wait_time)
        return self.read()

    def setUniform(self, name, value):
        if not self.isRunning():
            return False

        cmd = name
        if type(value) in (tuple, list):
            for val in value:
                cmd += ',' + str(val)
        else:
            cmd += ',' + str(value)
        return self.write(cmd)

    def setUniforms(self, uniforms_dict):
        if not self.isRunning():
            return False

        for uniform in uniforms_dict:
            if isinstance(uniforms_dict[uniform], dict):
                u_type = uniforms_dict[uniform]['type']
                u_value = uniforms_dict[uniform]['value']
                if u_type == 'int':
                    self.setUniform(uniform, int(u_value))
                elif u_type == 'float':
                    self.setUniform(uniform, float(u_value))
            else:
                self.setUniform(uniform, uniforms_dict[uniform])

    def getTime(self):
        if not self.isRunning():
            return False

        answer = self.write('time')
        if answer:
            if answer.replace('.', '', 1).isdigit():
                self.elapsed_time = float(answer)
        return self.elapsed_time

    def getDelta(self):
        if not self.isRunning():
            return False

        answer = self.write('delta')
        if answer:
            if answer.replace('.', '', 1).isdigit():
                self.delta = float(answer)
        return self.delta

    def getFPS(self):
        if not self.isRunning():
            return False

        answer = self.write('fps')
        if answer:
            if answer.replace('.', '', 1).isdigit():
                self.fps = float(answer)
        return self.fps

    def test(self, duration, record_from=0.0):
        if not self.isRunning():
            self.start()

        values = []
        time_start = time.time()
        while True:
            time_now = time.time()
            time_diff = time_now - time_start
            if time_diff >= record_from:
                if time_diff >= duration or self.isFinish():
                    break
            values.append({
                'time_diff': time_diff,
                'delta': self.getDelta(),
                'fps': self.getFPS()
            })
        return values

    def screenshot(self, filename):
        if not self.isRunning():
            return False

        answer = self.write('screenshot ' + filename)
        return answer
