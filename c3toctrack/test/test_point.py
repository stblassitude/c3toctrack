from c3toctrack import Point


def assert_equal_enough(angle: float, expected: int) -> None:
    assert int(round(angle, 0)) == expected


def test_angle_north() -> None:
    """
    Larger latitude means further north
    :return:
    """
    a = Point(53.2, 13.3)
    b = Point(53.3, 13.3)
    assert_equal_enough(a.angle(b), 0)


def test_angle_northeast() -> None:
    a = Point(53.2, 13.2)
    b = Point(53.3, 13.3)
    assert_equal_enough(a.angle(b), 45)


def test_angle_east() -> None:
    """
    Larger longitude means further east
    :return:
    """
    a = Point(53.3, 13.2)
    b = Point(53.3, 13.3)
    assert_equal_enough(a.angle(b), 90)


def test_angle_south() -> None:
    a = Point(53.3, 13.3)
    b = Point(53.2, 13.3)
    assert_equal_enough(a.angle(b), 180)


def test_angle_southwest() -> None:
    a = Point(53.3, 13.3)
    b = Point(53.2, 13.2)
    assert_equal_enough(a.angle(b), 225)


def test_angle_west():
    a = Point(53.3, 13.3)
    b = Point(53.3, 13.2)
    assert_equal_enough(a.angle(b), 270)


