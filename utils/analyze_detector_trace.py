import sys
import numpy as np
import pickle
import matplotlib.pyplot as plt
import csv
from queue import PriorityQueue
import heapq
from scipy.constants import c, pi, h


def waveguide_length_for_phase_shift(phi, wavelength, neff):
    phi = np.mod(phi, 2*pi)
    if phi == 0:
        phi = 2*pi

    length_req = phi * wavelength / (2 * pi * neff)

    return length_req


class Pulse():
    max_id = 0
    def __init__(self, power, duration, phase, tstart, wavelength=1.55e-6, sig_id=0):
        self.power = power
        self.duration = duration
        self.phase = phase
        self.tstart = tstart
        self.wavelength = wavelength
        if sig_id != 0:
            self.id = sig_id
            if self.id >= Pulse.max_id:
                Pulse.max_id = self.id + 1
        else:
            self.id = Pulse.max_id
            Pulse.max_id += 1

    @property
    def tend(self):
        return self.tstart + self.duration

    @property
    def energy(self):
        return self.power * self.duration

    def __str__(self):
        return f't={self.tstart}, P={self.power}, tau={self.duration}, phi={self.phase}, lambda={self.wavelength}, id={self.id}'

    def _cmp_key(self):
        return (self.tstart, self.id)

    def __lt__(self, rhs):
        return self._cmp_key() < rhs._cmp_key()

    # def __le__(self, rhs):
    #     return self.tstart <= rhs.tstart
    #
    # def __gt__(self, rhs):
    #     return rhs < self
    #
    # def __ge__(self, rhs):
    #     return rhs <= self

    def __eq__(self, rhs):
        #return self.tstart == rhs.tstart
        return self._cmp_key() == rhs._cmp_key()
        # return (self.power == rhs.power
        #     and self.duration == rhs.duration
        #     and self.phase == rhs.phase
        #     and self.tstart == rhs.tstart
        #     and self.wavelength == rhs.wavelength
        #     )
    
    # def __neq__(self, rhs):
    #     return not self == rhs

    def intersects(lhs, rhs):
        # sort by tstart
        #pulses = sorted([lhs, rhs], key=lambda x: x.tstart)

        # if they don't interfere
        if lhs.tstart <= rhs.tstart and lhs.tend - rhs.tstart > 1e-20:
            return True
        if rhs.tstart <= lhs.tstart and rhs.tend - lhs.tstart > 1e-20:
            return True

        return False

    def __add__(lhs, rhs):
        # sort by tstart
        pulses = sorted([lhs, rhs])

        assert(pulses[0].tstart <= pulses[1].tstart)

        # if they don't interfere, return both pulses
        if pulses[0].tend <= pulses[1].tstart:
            return pulses

        if pulses[0].wavelength != pulses[1].wavelength:
            raise ValueError('Cannot handle addition of pulses of different frequencies with this class')

        #print(f'Summing pulses with timestamps {pulses[0].tstart} and {pulses[1].tstart}')
        wavelength = pulses[0].wavelength

        phi1 = pulses[0].phase
        P1 = pulses[0].power
        A1 = np.sqrt(P1)

        phi2 = pulses[1].phase
        P2 = pulses[1].power
        A2 = np.sqrt(P2)

        dphi = phi2 - phi1

        #print(f'dphi = {np.mod(dphi,2*pi)/pi:.3f}*pi')
        #print(f'dphi/2 = {np.mod(dphi/2,2*pi)/pi:.3f}*pi')
        #print(f'cos(dphi/2) = {np.cos(dphi/2)}')
        A_sum = (A1 + A2) * np.cos(dphi/2) - 1j * (A1 - A2) * np.sin(dphi/2)
        #P_sum = np.real(A_sum * np.conj(A_sum))
        P_sum = np.abs(A_sum) ** 2
        phi_sum = (phi1 + phi2) / 2 + np.angle(A_sum)

        last_pulse = 1
        if pulses[0].tend > pulses[1].tend:
            last_pulse = 0

        tstart_pre = pulses[0].tstart
        tstart_sum = pulses[1].tstart
        tstart_post = pulses[not last_pulse].tend

        duration_pre = tstart_sum - tstart_pre
        duration_sum = tstart_post - tstart_sum
        duration_post = pulses[last_pulse].tend - tstart_post

        P_post = [P1, P2][last_pulse]
        phi_post = [phi1, phi2][last_pulse]

        assert(duration_pre >= 0)
        assert(duration_sum >= 0)
        assert(duration_post >= 0)

        result = []
        if duration_pre > 0:
            result.append(Pulse(P1, duration_pre, phi1, tstart_pre, wavelength))

        result.append(Pulse(P_sum, duration_sum, phi_sum, tstart_sum, wavelength))

        if duration_post > 0:
            result.append(Pulse(P_post, duration_post, phi_post, tstart_post, wavelength))

        return result

    @classmethod
    def sort_by_tstart(cls, pulses):
        return sorted(pulses)


class PulseAggreg():
    def __init__(self, pulses):
        self.reduced = False
        self.pulses = pulses
        heapq.heapify(self.pulses)
        self.sort_pulses_by_tstart()
        # print(str(self))
    
    def sort_pulses_by_tstart(self):
        #heapq.heapify(self.pulses)
        self.pulses = Pulse.sort_by_tstart(self.pulses)
        pass

    @property
    def has_intersections(self):
        if self.reduced:
            return False

        heapq.heapify(self.pulses)
        for i,p in enumerate(self.pulses[:-1]):
            p2 = self.pulses[i+1]
            if p.intersects(p2):
                return True
        # for i,p in enumerate(self.pulses[:-1]):
        #     for i2,p2 in enumerate(self.pulses[i+1:]):
        #         if p.intersects(p2):
        #             return True
        return False

    def reduce_pulses_coherent(self):
        if not self.has_intersections:
            self.reduced = True
            return

        pulses = self.pulses
        pulses_new = []
        
        n = len(pulses)
        i = 0
        print('Reducing to independent pulses...')
        while len(pulses) > 1:
            print(f'\r{100*(1-len(pulses)/n):.0f}% ({len(pulses)} pulses left)', end='')
            #pulses = Pulse.sort_by_tstart(pulses)
            p0 = heapq.heappop(pulses)
            p1 = heapq.heappop(pulses)

            if p0.intersects(p1):
                resulting_pulses = p0 + p1
                #heapq.heappop(pulses)
                # print(p0)
                # print(p1)
                # print('-->')
                for px in resulting_pulses:
                    heapq.heappush(pulses, px)
                #heapq.heappush(pulses_new, resulting_pulses[0])
                    # print(px)
                # print(' ')
            else:
                heapq.heappush(pulses_new, p0)
                heapq.heappush(pulses, p1)


        if len(pulses) == 1:
            heapq.heappush(pulses_new, heapq.heappop(pulses))

        print(f'\r{100*(1-len(pulses)/n):.0f}% ({len(pulses)} pulses left)')
        self.pulses = pulses_new

        print('Checking...')
        if self.has_intersections:
            raise RuntimeError('Impossible to reduce pulse list')

        with open('NonIntersectingPulses.obj', 'wb') as f:
            pickle.dump(self, f)

        print('Done')
        self.reduced = True

    def __str__(self):
        ret = '\n'.join([str(p) for p in list(self.pulses)])
        return ret

    @property
    def energy(self):
        if self.has_intersections:
            self.reduce_pulses_coherent()
        return np.sum([p.energy for p in self.pulses])


    def to_waveform(self, dt, coherent=True, tmax=None):
        if self.has_intersections:
            self.reduce_pulses_coherent()

        #print(np.min([p.duration for p in self.pulses]))
        #print(np.max([p.power for p in self.pulses]))
        #print(np.min([p.power for p in self.pulses]))
        #print(str(self))
        #print(self.energy)

        if tmax == None:
            tmax = self.pulses[-1].tend

        t = np.arange(0, tmax, dt)
        Pout = np.zeros(len(t))

        for p in self.pulses:
            # itmin = int(np.ceil(p.tstart / dt))
            # itmax = int(np.ceil(p.tend / dt) + 1)
            Pout[np.logical_and(t >= p.tstart, t < p.tend)] = p.power
            #Pout[itmin:itmax] = p.power

        return t, Pout

    def to_waveform_2(self, coherent=True):
        if self.has_intersections:
            self.reduce_pulses_coherent()

        #print(self.energy)

        t = [0]
        Pout = [0]

        for p in self.pulses:
            if p.duration < 1e-20:
                continue
            if p.tstart - t[-1] > 1e-20:
                t.append(t[-1])
                Pout.append(0)
                t.append(p.tstart)
                Pout.append(0)
            t.append(p.tstart)
            Pout.append(p.power)
            t.append(p.tend)
            Pout.append(p.power)

        return np.array(t), np.array(Pout)

    def plot_waveform(self, dt, title=None, tmax=None):
        print(f'Mapping to timeseries ({len(self.pulses)} pulses to process)')
        #t, Pout = self.to_waveform(dt, tmax=tmax)
        t, Pout = self.to_waveform_2()
        print('Done')

        with open('Pout.obj', 'wb') as f:
            pickle.dump((t,Pout), f)

        linespec='-'
        if title is not None and '.bk' in title:
            linespec='--'
        plt.plot(1e9*t, 1e3*Pout, linespec, label=title)
        if tmax is not None:
            plt.xlim([0, 1e9*tmax])
        #plt.title(title)
        plt.xlabel('t (ns)')
        plt.ylabel('Pout (mW)')
        plt.savefig('Pout.png', dpi=300)
        plt.grid('major')
        plt.grid('minor')
        plt.legend()


def main(filename='detector_trace.txt', override_lambda=None):
    pulses = []
    tmax = 0
    taumin = 1

    with open(filename, 'r') as f:
        print('Reading trace file...')
        fieldnames = [h.strip() for h in next(csv.reader(f))]

        reader = csv.DictReader(f, fieldnames=fieldnames)
        for (i, row) in enumerate(reader):
            # if i > 470:
            #     break
            #power = np.round(float(row['P (W)']), 12)
            power = float(row['P (W)'])
            #tau = np.round(float(row['tau (s)']), 16)
            tau = float(row['tau (s)'])
            phase = float(row['phi (rad)'])
            #tstart = np.round(float(row['t (s)']), 16)
            tstart = float(row['t (s)'])
            wavelength = np.round(float(row['lambda (m)']), 12)
            if override_lambda is not None:
                wavelength = override_lambda
            sig_id = int(row['id'])

            #if tau < 1e-16: continue

            #print(wavelength)
            p = Pulse(power, tau, phase, tstart, wavelength, sig_id)
            pulses.append(p)
            #tmax = max(tmax, p.tend)
            taumin = min(taumin, tau)

    print(f'Smallest pulse duration: {taumin}')
    pulses = PulseAggreg(pulses)
    print(f'Some pulses intersect: {pulses.has_intersections}')
    pulses.reduce_pulses_coherent()

    #t = np.arange(0, tmax, 10e-12)
    #dt = 10e-12
    dt = 20e-12
    print(f'Total energy: {pulses.energy}')
    return pulses

def create_bitstream(n):
    rng = np.random.default_rng()
    bitstream = rng.integers(low=0, high=2, size=n)
    return bitstream

def xor_bitstream(bitstream):
    bitstream_a = bitstream
    bitstream_b = [0, *bitstream_a]
    bitstream = [a ^ b for a, b in zip(bitstream_a, bitstream_b)]
    return bitstream

def bitstream_as_values(bitstream, nbits_per_value=8, pad_with=0):
    padding_length = nbits_per_value - (len(bitstream) % nbits_per_value)
    bitstream = [*bitstream, *[pad_with for i in range(padding_length)]]
    
    values = []
    for i in range(len(bitstream) // nbits_per_value):
        values.append(int(''.join([str(b) for b in bitstream[i:i+nbits_per_value]]), 2))
    return values

def test_bitstreams():
    bs = create_bitstream(20)
    bs_xor = xor_bitstream(bs)
    values_1bit = bitstream_as_values(bs, 1)
    values_2bit = bitstream_as_values(bs, 2)

    plt.figure()
    plt.subplot(2,2,1)
    plt.plot(bs)
    plt.subplot(2,2,2)
    plt.plot(bs_xor)
    plt.subplot(2,2,3)
    plt.plot(values_1bit)
    plt.subplot(2,2,4)
    plt.plot(values_2bit)
    plt.show()

def create_random_bitstream_file(filename, nbits=3000, nbits_per_value=8):
    bs = create_bitstream(nbits)
    #bs_xor = xor_bitstream(bs)
    values = bitstream_as_values(bs, nbits_per_value)
    with open(filename, 'w') as f:
        f.write(' '.join([str(v) for v in values]))

def compare_Pout_vecs():
    with open('Pout_nosort.obj', 'r') as f:
        data = pickle.load(f)
    t_sort, Pout_sort = pickle.load(open('Pout_sort.obj'))

    assert(len(Pout_sort) == len(Pout_nosort))
    for t, p1, p2 in zip(t_nosort, Pout_nosort, Pout):
        assert(p1 == p2)

if __name__ == '__main__':
    print(waveguide_length_for_phase_shift(2*pi, 1550e-9, 2.2111))
    print(waveguide_length_for_phase_shift(pi, 1550e-9, 2.2111))
    print(waveguide_length_for_phase_shift(1, 1550e-9, 2.2111))

    a = waveguide_length_for_phase_shift(2*pi, 1551e-9, 2.2111)
    b = waveguide_length_for_phase_shift(pi, 1551e-9, 2.2111)
    print(a)
    print(b)
    print(500*a+b)
    #exit(0)
    # print(700*waveguide_length_for_phase_shift(2*pi, 1550e-9, 2.2111))
    # compare_Pout_vecs()
    if len(sys.argv) > 1:
        P = []
        for fn in sys.argv[1:]:
            P.append(main(filename=fn))
            P[-1].plot_waveform(1e-13,title=fn, tmax=2.5e-9)

        if len(sys.argv) > 2:
            plt.figure()
            f0 = P[0].to_waveform(1e-13, tmax=2.5e-9)
            f1 = P[1].to_waveform(1e-13, tmax=2.5e-9)
            plt.plot(f0[0], f1[1] - f0[1])
        plt.show()
    else:
        main()
        plt.show()
    #test_bitstreams()
    #create_random_bitstream_file('bitstream.txt', 3000)
