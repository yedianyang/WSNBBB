class Filter:
    def __init__(self):
        self.data_filter = [0, 0, 0, 0, 0, 0, 0, 0]

    def average_filter(self, signalIn):
        self.data_filter.pop(len(self.data_filter)-1)
        self.data_filter.insert(0, signalIn)
        return sum(self.data_filter)/len(self.data_filter)


def mapping(value, originalMin, originalMax, targetMin, targetMax):
    # Figure out how 'wide' each range is
    originalSpan = originalMax - originalMin
    targetSpan = targetMax - targetMin

    # Convert the left range into a 0-1 range (float)
    valueScaled = float(value - originalMin) / float(originalSpan)

    # Convert the 0-1 range into a value in the right range.
    return float(targetMin + (valueScaled * targetSpan))