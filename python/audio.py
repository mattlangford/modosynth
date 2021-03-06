import numpy as np
import matplotlib.pyplot as plt
import wave
import struct

SampleRate = 44000
SampleTime = 1 / SampleRate
BatchSize = 128
BatchTime = BatchSize / SampleRate

class Signal(object):
    def __init__(self):
        self.data = []
        self.phase = 0

    def sum(self, signal):
        for (batch, rhs_batch) in zip(self.data, signal.data):
            batch += rhs_batch

    def populate_samples(self, f):
        phase_inc = 2 * np.pi * f / SampleRate
        x = np.arange(0, BatchSize, phase_inc)
        self.data.append(np.sin(self.phase + x))
        print (f"phase: {self.phase}, inc: {phase_inc}, final: {x[-1] + phase_inc}")
        self.phase += x[-1] + phase_inc

    def plot(self):
        full_data = []
        for d in self.data:
            full_data.extend(d.tolist())

        x = np.arange(0, len(full_data)) / SampleRate

        plt.plot(x, np.array(full_data))

    def fft(self, max_freq):
        full_data = []
        for d in self.data:
            full_data.extend(d.tolist())
        full_data.extend(np.zeros(SampleRate))

        fft_wave = np.fft.rfft(full_data)
        fft_freq = np.fft.rfftfreq(len(full_data), d=1.0/SampleRate)

        print (max(np.abs(fft_wave)), min(np.abs(fft_wave)))
        plt.plot(fft_freq[:max_freq], np.abs(fft_wave[:max_freq]))

    def write(self, name):
        full_data = []
        for d in self.data:
            full_data.extend(d.tolist())

        f = wave.open(name, 'wb')
        f.setnchannels(1)
        f.setsampwidth(2)
        f.setframerate(SampleRate)
        f.setnframes(len(full_data))

        for d in full_data:
            value = int((2 ** 15 - 1) * max(min(d, 1.0), -1.0))
            data = struct.pack('<h', value)
            f.writeframesraw(data)

        f.close()

def intermediate(fs, f0, db_gain, s):
    A = 10. ** (db_gain / 40.)
    w = 2 * np.pi * f0 / fs
    alpha = 0.5 * np.sin(w) * np.sqrt((A + 1 / A) * (1 / s - s) + 2)
    print (f"Q: {1 / np.sqrt((A + 1 / A) * (1 / s - s) + 2)}")
    return w, alpha

# https://webaudio.github.io/Audio-EQ-Cookbook/audio-eq-cookbook.html for the coeff calculations
class BiQuadFilter(object):
    def __init__(self, coeff):
        print (f"BiQuadFilter Coefficients: {coeff}")
        self.coeff = coeff

    @staticmethod
    def lpf(fs, f0, db_gain, s):
        w, alpha = intermediate(fs, f0, db_gain, s)
        b0 = (1 - np.cos(w)) * 0.5
        b1 = 2 * b0
        b2 = b0
        a0 = 1 + alpha
        a1 = -2 * np.cos(w)
        a2 = 1 - alpha
        return BiQuadFilter([b0 / a0, b1 / a0, b2 / a0, a0 / a0, a1 / a0, a2 / a0])

    @staticmethod
    def hpf(fs, f0, db_gain, s):
        w, alpha = intermediate(fs, f0, db_gain, s)
        b0 = (1 + np.cos(w)) * 0.5
        b1 = -2 * b0
        b2 = b0
        a0 = 1 + alpha
        a1 = -2 * np.cos(w)
        a2 = 1 - alpha
        return BiQuadFilter([b0 / a0, b1 / a0, b2 / a0, a0 / a0, a1 / a0, a2 / a0])

    def process(self, signal):
        xn_1 = 0
        xn_2 = 0
        yn_1 = 0
        yn_2 = 0

        b0, b1, b2, a0, a1, a2 = np.copy(self.coeff)

        output = Signal()
        for batch in signal.data:
            output_batch = []
            for sample in batch:
                xn = sample
                yn = b0 * xn + b1 * xn_1 + b2 * xn_2 - a1 * yn_1 - a2 * yn_2

                output_batch.append(yn)

                xn_2 = xn_1
                xn_1 = xn

                yn_2 = yn_1
                yn_1 = yn

            output.data.append(np.array(output_batch))

        return output


s = Signal()

t = np.linspace(0.0, 1.0, SampleRate)
data = np.zeros_like(t)
#for f in range(100, 10000, 10):
data = np.cos(2 * np.pi * 1000 * t)
data += np.cos(2 * np.pi * 100 * t)

s.data.append(data)

f = BiQuadFilter.hpf(SampleRate, 3000, 3, 0.1)
s.fft(2000)
plt.yscale('log',base=10)

s = f.process(s)
s.fft(2000)

# s.write("/tmp/out.wav")
#s.fft(10000)
plt.show()

