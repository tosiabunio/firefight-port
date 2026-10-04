#!/usr/bin/env python3
"""Writes an input script that records a golden demo of one mission (tests/record_demo.cmake).

The script starts from the title: Return opens the mission screen, Down selects the mission,
Page Up opens the demo menu (debug mode, cwdiags=extended) and Return records the mission. Then it
flies for a while with random but fixed keys: forward, turns, both fire buttons, strafing, turbo,
weapon and inventory keys. It never presses Escape, the function keys or Alt+X.

  flight.py <mission index in missions_order> <seed> <frames of flight> > <script>
"""
import random
import sys

START = 430  # the mission is running by then (it starts recording about frame 400)


def main():
    mission, seed, frames = int(sys.argv[1]), int(sys.argv[2]), int(sys.argv[3])
    rng = random.Random(seed)
    events = []  # (frame, order, line)

    def tap(frame, key):
        events.append((frame, 0, f'key {key} down'))
        events.append((frame + 2, 0, f'key {key} up'))

    tap(250, 'Return')
    for i in range(mission):
        tap(300 + 10 * i, 'Down')
    menu = 300 + 10 * mission + 20
    tap(menu, 'PageUp')
    tap(menu + 20, 'Return')
    events.append((menu + 20, -1, f'log recording mission {mission}'))
    end = START + frames

    def segments(key, on, off, first):
        frame = START + first
        while frame < end:
            length = rng.randint(*on)
            events.append((frame, 1, f'key {key} down'))
            events.append((min(frame + length, end), 0, f'key {key} up'))
            frame += length + rng.randint(*off)

    segments('Up', (60, 200), (10, 40), 0)
    segments('Left', (5, 30), (40, 120), rng.randint(10, 60))
    segments('Right', (5, 30), (40, 120), rng.randint(10, 60))
    segments('Space', (10, 40), (20, 60), rng.randint(0, 30))
    segments('Left Ctrl', (5, 20), (100, 300), rng.randint(50, 150))
    segments('Z', (10, 30), (150, 400), rng.randint(100, 300))
    segments('X', (10, 30), (150, 400), rng.randint(100, 300))
    segments('Left Shift', (20, 60), (150, 400), rng.randint(100, 300))
    for _ in range(frames // 150):
        tap(rng.randint(START, end - 3), rng.choice(['1', '2', '3', '4', '5', '6', 'PageUp',
                                                     'PageDown', 'Keypad +', 'Keypad -', 'Return']))
    print(f'# Golden demo: mission {mission} of missions_order, seed {seed}, {frames} frames of flight.')
    print('# Written by tests/demos/flight.py; recorded by tests/record_demo.cmake.')
    for frame, _, line in sorted(events):
        print(f'{frame} {line}')


if __name__ == '__main__':
    main()
