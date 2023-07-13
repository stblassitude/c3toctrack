from math import sqrt
from c3toctrack import Point


# 13°18'30 - 13°18'20 == 186.3 m
# 53°02'02 - 53°01'57 == 154.5 m

class Track:
    meter_per_degree_lon=1.4910240353067286e-05
    meter_per_degree_lat=8.989572096375368e-06
    def __init__(self, name):
        self.name = name
        self.points = []
        self.trackmiles = []
        self.distances = []

    def add(self, point:Point):
        if len(self.points) > 0:
            previous = self.points[len(self.points)-1]
            if len(self.trackmiles) > 0:
                tm = self.trackmiles[len(self.trackmiles)-1]
            else:
                tm = 0
            lon = abs(previous.lon - point.lon) / self.meter_per_degree_lon
            lat = abs(previous.lat - point.lat) / self.meter_per_degree_lat
            d = sqrt(lon*lon+lat*lat)
            self.distances.append(d)
            self.trackmiles.append(tm+d)
        self.points.append(point)
