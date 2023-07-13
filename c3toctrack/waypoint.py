class Waypoint:
    """
    Abstract base class.
    """
    def __init__(self, lat:float, lon:float, name:str, trackmarker:float):
        """
        Create a waypoint.
        :param lat: latitude of waypoint
        :param lon: longitude of waypoint
        :param name: name
        :param trackmarker: position of the waypoing on the line, in meters. The more common English expression would be mile marker. 
        """
        self.lat = lat
        self.lon = lon
        self.name = name
        self.trackmarker = trackmarker
        self.type = "wp"


class LevelCrossing(Waypoint):
    """
    A level crossing, in German "Bahnübergang" or "Bü".
    """
    def __init__(self, lat:float, lon:float, name:str, trackmarker:float):
        super().__init__(lat, lon, name, trackmarker)
        self.type = "Bü"


class Stop(Waypoint):
    """
    A stop, in German "Haltepunkt" or "Hp". Trains can stop here, but it is not a station.
    """
    def __init__(self, lat:float, lon:float, name:str, trackmarker:float):
        super().__init__(lat, lon, name, trackmarker)
        self.type = "Hp"


class Station(Stop):
    """
    A station, in German "Bahnhof" or "Bf". The legal definition of a station is a train facility with at least one
    turnout, where trains are allowed to start, end, stop, overtake, cross, or turn around. A Haltepunkt therefor does
    not fulfill these requirements, but trains can still stop there.
    """
    def __init__(self, lat:float, lon:float, name:str, trackmarker:float):
        super().__init__(lat, lon, name, trackmarker)
        self.type = "Bf"


def makeWaypoint(lat:float, lon:float, trackmarker:float, label:str) -> Waypoint:
    (type, name) = label.split(' ', 1)
    match type:
        case "Bf":
            return Station(lat, lon, name, trackmarker)
        case "Bü":
            return LevelCrossing(lat, lon, name, trackmarker)
        case "Hp":
            return Stop(lat, lon, name, trackmarker)
        case _:
            raise Exception(f'Unknown waypoint type {type}')