from math import sqrt
from .point import Point
from .waypoint import makeWaypoint


# 13°18'30 - 13°18'20 == 186.3 m
# 53°02'02 - 53°01'57 == 154.5 m

class Track:
    """
    A track combines multiple Points.
    """

    def __init__(self, name):
        self.name = name
        self.points = []

    def add(self, lat: float, lon: float, name: str, start: float):
        point = Point(lat, lon)
        if len(self.points) > 0:
            previous = self.points[len(self.points) - 1]
            start = previous.trackmarker
            point.distance = Point.distance(previous, point)
        point.trackmarker = start + point.distance
        if name is not None:
            point.waypoint = makeWaypoint(point.lat, point.lon, int(point.trackmarker), name)
        self.points.append(point)
