class Point:
    """
    A point along a track. Not to be confused with a turnout or switch.
    """

    def __init__(self, lat: float, lon: float):
        self.lat = lat
        self.lon = lon
        self.distance = 0
        self.trackmarker = 0
        self.waypoint = None
