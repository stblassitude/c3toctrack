from math import sqrt


class Point:
    """
    A point along a track. Not to be confused with a turnout or switch.
    """

    degree_per_meter_lon = 1.4910240353067286e-05
    degree_per_meter_lat = 8.989572096375368e-06

    def __init__(self, lat: float, lon: float):
        self.lat = lat
        self.lon = lon
        self.distance = 0
        self.trackmarker = 0
        self.waypoint = None

    @staticmethod
    def distance(a: "Point", b: "Point"):
        lon = abs(a.lon - b.lon) / Point.degree_per_meter_lon
        lat = abs(a.lat - b.lat) / Point.degree_per_meter_lat
        return sqrt(lon * lon + lat * lat)

    @staticmethod
    def nearest_distance(ab: float, bc: float, ca: float):
        """
        Given a triangle a, b, c, return the relative distance of c projected to the base line of a-b.
        """
        s = (ab + bc + ca) / 2
        hc = 2 * sqrt(s * (s - ab) * (s - bc) * (s - ca)) / ab
        return sqrt(hc * hc + ca * ca)
