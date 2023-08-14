from math import atan2, sqrt, pi


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

    def angle(self, b: "Point") -> float:
        """
        Compute the angle from self to point b
        :param b: the other point
        :return: angle in degrees
        """
        a = atan2(b.lon - self.lon, b.lat - self.lat) * 180 / pi
        if a < 0:
            a += 360
        return a

    @staticmethod
    def distance(a: "Point", b: "Point") -> float:
        """
        Compute the distance between a and b in meters
        :param a:
        :param b:
        :return:
        """
        lon = abs(a.lon - b.lon) / Point.degree_per_meter_lon
        lat = abs(a.lat - b.lat) / Point.degree_per_meter_lat
        return sqrt(lon * lon + lat * lat)

    @staticmethod
    def nearest_distance(ab: float, bc: float, ca: float) -> float:
        """
        Given a triangle a, b, c, return the relative distance of c projected to the base line of a-b.
        """
        if ab == 0:
                return 0
        s = (ab + bc + ca) / 2
        hc = 2 * sqrt(s * (s - ab) * (s - bc) * (s - ca)) / ab
        return sqrt(hc * hc + ca * ca)
