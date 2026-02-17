from typing import Optional

from vispy.scene import visuals


class ScrollBar:
    """A horizontal scrollbar widget for Vispy Scene."""

    def __init__(self, parent, size_callback):
        self.parent = parent
        self.visible = True
        self.get_total_size = size_callback

        self.thumb = visuals.Rectangle(
            center=(0, 0, 0),
            width=20,
            height=18,
            color=(0.6, 0.6, 0.6, 1),
            parent=parent,
        )
        self.thumb.order = 100

        self.width = 100
        self.height = 20
        self.is_dragging = False
        self.last_mouse_x = 0

    def update_layout(self, x, y, width, height):
        self.width = width
        self.height = height

        total, visible_size, offset = self.get_total_size()
        if total <= visible_size:
            thumb_w = width
            thumb_x = x
        else:
            ratio = visible_size / total
            thumb_w = max(20, width * ratio)

            max_offset = total - visible_size
            scroll_range_px = width - thumb_w
            if max_offset > 0:
                normalized_pos = offset / max_offset
                thumb_x = x + scroll_range_px - (normalized_pos * scroll_range_px)
            else:
                thumb_x = x

        thumb_h = height - 4
        if thumb_h < 1:
            thumb_h = 1

        thumb_cx = thumb_x + thumb_w / 2.0
        thumb_cy = y + height / 2.0

        self.thumb.center = (thumb_cx, thumb_cy, 0)
        self.thumb.width = thumb_w
        self.thumb.height = thumb_h

        self.thumb_rect_x = (thumb_x, thumb_x + thumb_w)
        self.parent.update()

    def handle_mouse_press(self, event, x_rel):
        """Returns True if consumed."""
        if not hasattr(self, "thumb_rect_x"):
            return False

        thumb_left, thumb_right = self.thumb_rect_x
        if thumb_left <= x_rel <= thumb_right:
            self.is_dragging = True
            self.last_mouse_x = event.pos[0]
            return True
        return False

    def handle_mouse_move(self, event) -> Optional[float]:
        """Returns new normalized offset (0.0 to 1.0) if dragging, else None."""
        if not self.is_dragging:
            return None

        delta_x = event.pos[0] - self.last_mouse_x
        self.last_mouse_x = event.pos[0]

        total, visible_size, _ = self.get_total_size()
        max_offset = total - visible_size

        if max_offset <= 0:
            return 0.0

        thumb_w = self.thumb.width
        scroll_range_px = self.width - thumb_w
        if scroll_range_px <= 0:
            return None

        offset_delta = -(delta_x / scroll_range_px) * max_offset
        return offset_delta

    def handle_mouse_release(self, event):
        self.is_dragging = False
