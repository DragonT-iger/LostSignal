"""
UMG Tools for Unreal MCP.

This module provides tools for creating and manipulating UMG Widget Blueprints in Unreal Engine.
"""

import logging
from typing import Dict, List, Any
from mcp.server.fastmcp import FastMCP, Context

# Get logger
logger = logging.getLogger("UnrealMCP")

def register_umg_tools(mcp: FastMCP):
    """Register UMG tools with the MCP server."""

    @mcp.tool()
    def create_umg_widget_blueprint(
        ctx: Context,
        widget_name: str,
        parent_class: str = "UserWidget",
        path: str = "/Game/UI"
    ) -> Dict[str, Any]:
        """
        Create a new UMG Widget Blueprint.
        
        Args:
            widget_name: Name of the widget blueprint to create
            parent_class: Parent class for the widget (default: UserWidget)
            path: Content browser path where the widget should be created
            
        Returns:
            Dict containing success status and widget path
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "name": widget_name,        # C++ plugin expects "name"
                "parent_class": parent_class,
                "path": path
            }
            
            logger.info(f"Creating UMG Widget Blueprint with params: {params}")
            response = unreal.send_command("create_umg_widget_blueprint", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Create UMG Widget Blueprint response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error creating UMG Widget Blueprint: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_text_block_to_widget(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0],
        font_size: int = 12,
        color: List[float] = [1.0, 1.0, 1.0, 1.0],
        parent_name: str = "",
        auto_wrap_text: bool = False,
        justification: str = "left",
        size_rule: str = "",
        fill_value: float = 1.0,
        h_align: str = "",
        v_align: str = "",
        padding: List[float] = []
    ) -> Dict[str, Any]:
        """
        Add a Text Block widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name to give the new Text Block
            text: Initial text content
            position: [X, Y] position (Canvas Panel only)
            size: [Width, Height] (Canvas Panel only)
            font_size: Font size in points
            color: [R, G, B, A] color values (0.0 to 1.0)
            parent_name: Name of parent widget to attach to (empty = root)
            auto_wrap_text: Enable automatic text wrapping
            justification: Text alignment - "left", "center", "right"
            size_rule: Slot size rule - "fill" or "" (auto)
            fill_value: Fill coefficient when size_rule is "fill"
            h_align: Horizontal alignment - "fill", "left", "center", "right"
            v_align: Vertical alignment - "fill", "top", "center", "bottom"
            padding: [L, T, R, B] slot padding
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": widget_name,
                "widget_name": text_block_name,
                "text": text,
                "position": position,
                "size": size,
                "font_size": font_size,
                "color": color,
                "auto_wrap_text": auto_wrap_text,
                "justification": justification,
            }
            if parent_name:
                params["parent_name"] = parent_name
            if size_rule:
                params["size_rule"] = size_rule
                params["fill_value"] = fill_value
            if h_align:
                params["h_align"] = h_align
            if v_align:
                params["v_align"] = v_align
            if padding:
                params["padding"] = padding

            response = unreal.send_command("add_text_block_to_widget", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response

        except Exception as e:
            return {"success": False, "message": f"Error adding Text Block: {e}"}

    @mcp.tool()
    def add_button_to_widget(
        ctx: Context,
        widget_name: str,
        button_name: str,
        text: str = "",
        position: List[float] = [0.0, 0.0],
        size: List[float] = [200.0, 50.0],
        font_size: int = 12,
        color: List[float] = [1.0, 1.0, 1.0, 1.0],
        background_color: List[float] = [0.1, 0.1, 0.1, 1.0],
        parent_name: str = "",
        size_rule: str = "",
        fill_value: float = 1.0,
        h_align: str = "",
        v_align: str = "",
        padding: List[float] = []
    ) -> Dict[str, Any]:
        """
        Add a Button widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            button_name: Name to give the new Button
            text: Text to display on the button
            position: [X, Y] position (Canvas Panel only)
            size: [Width, Height] (Canvas Panel only)
            font_size: Font size for button text
            color: [R, G, B, A] text color values (0.0 to 1.0)
            background_color: [R, G, B, A] button background color values (0.0 to 1.0)
            parent_name: Name of parent widget (empty = root)
            size_rule: Slot size rule - "fill" or "" (auto)
            fill_value: Fill coefficient when size_rule is "fill"
            h_align: Horizontal alignment - "fill", "left", "center", "right"
            v_align: Vertical alignment - "fill", "top", "center", "bottom"
            padding: [L, T, R, B] slot padding
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": widget_name,
                "widget_name": button_name,
                "text": text,
                "position": position,
                "size": size,
                "font_size": font_size,
                "color": color,
                "background_color": background_color,
            }
            if parent_name:
                params["parent_name"] = parent_name
            if size_rule:
                params["size_rule"] = size_rule
                params["fill_value"] = fill_value
            if h_align:
                params["h_align"] = h_align
            if v_align:
                params["v_align"] = v_align
            if padding:
                params["padding"] = padding

            response = unreal.send_command("add_button_to_widget", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response

        except Exception as e:
            return {"success": False, "message": f"Error adding Button: {e}"}

    @mcp.tool()
    def bind_widget_event(
        ctx: Context,
        widget_name: str,
        widget_component_name: str,
        event_name: str,
        function_name: str = ""
    ) -> Dict[str, Any]:
        """
        Bind an event on a widget component to a function.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            widget_component_name: Name of the widget component (button, etc.)
            event_name: Name of the event to bind (OnClicked, etc.)
            function_name: Name of the function to create/bind to (defaults to f"{widget_component_name}_{event_name}")
            
        Returns:
            Dict containing success status and binding information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            # If no function name provided, create one from component and event names
            if not function_name:
                function_name = f"{widget_component_name}_{event_name}"
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": widget_component_name,
                "event_name": event_name,
                "function_name": function_name
            }
            
            logger.info(f"Binding widget event with params: {params}")
            response = unreal.send_command("bind_widget_event", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Bind widget event response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error binding widget event: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_widget_to_viewport(
        ctx: Context,
        widget_name: str,
        z_order: int = 0
    ) -> Dict[str, Any]:
        """
        Add a Widget Blueprint instance to the viewport.
        
        Args:
            widget_name: Name of the Widget Blueprint to add
            z_order: Z-order for the widget (higher numbers appear on top)
            
        Returns:
            Dict containing success status and widget instance information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "z_order": z_order
            }
            
            logger.info(f"Adding widget to viewport with params: {params}")
            response = unreal.send_command("add_widget_to_viewport", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Add widget to viewport response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error adding widget to viewport: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def set_text_block_binding(
        ctx: Context,
        widget_name: str,
        text_block_name: str,
        binding_property: str,
        binding_type: str = "Text"
    ) -> Dict[str, Any]:
        """
        Set up a property binding for a Text Block widget.
        
        Args:
            widget_name: Name of the target Widget Blueprint
            text_block_name: Name of the Text Block to bind
            binding_property: Name of the property to bind to
            binding_type: Type of binding (Text, Visibility, etc.)
            
        Returns:
            Dict containing success status and binding information
        """
        from unreal_mcp_server import get_unreal_connection
        
        try:
            unreal = get_unreal_connection()
            if not unreal:
                logger.error("Failed to connect to Unreal Engine")
                return {"success": False, "message": "Failed to connect to Unreal Engine"}
            
            params = {
                "blueprint_name": widget_name,
                "widget_name": text_block_name,
                "binding_property": binding_property,
                "binding_type": binding_type
            }
            
            logger.info(f"Setting text block binding with params: {params}")
            response = unreal.send_command("set_text_block_binding", params)
            
            if not response:
                logger.error("No response from Unreal Engine")
                return {"success": False, "message": "No response from Unreal Engine"}
            
            logger.info(f"Set text block binding response: {response}")
            return response
            
        except Exception as e:
            error_msg = f"Error setting text block binding: {e}"
            logger.error(error_msg)
            return {"success": False, "message": error_msg}

    @mcp.tool()
    def add_vertical_box_to_widget(
        ctx: Context,
        widget_name: str,
        vbox_name: str,
        parent_name: str = "",
        size_rule: str = "",
        fill_value: float = 1.0,
        h_align: str = "",
        v_align: str = "",
        padding: List[float] = []
    ) -> Dict[str, Any]:
        """
        Add a Vertical Box layout widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            vbox_name: Name to give the new Vertical Box
            parent_name: Name of parent widget (empty = root)
            size_rule: Slot size rule - "fill" or "" (auto)
            fill_value: Fill coefficient when size_rule is "fill"
            h_align: Horizontal alignment in parent slot
            v_align: Vertical alignment in parent slot
            padding: [L, T, R, B] slot padding
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"blueprint_name": widget_name, "widget_name": vbox_name}
            if parent_name:
                params["parent_name"] = parent_name
            if size_rule:
                params["size_rule"] = size_rule
                params["fill_value"] = fill_value
            if h_align:
                params["h_align"] = h_align
            if v_align:
                params["v_align"] = v_align
            if padding:
                params["padding"] = padding

            response = unreal.send_command("add_vertical_box_to_widget", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response

        except Exception as e:
            return {"success": False, "message": f"Error adding Vertical Box: {e}"}

    @mcp.tool()
    def add_horizontal_box_to_widget(
        ctx: Context,
        widget_name: str,
        hbox_name: str,
        parent_name: str = "",
        size_rule: str = "",
        fill_value: float = 1.0,
        h_align: str = "",
        v_align: str = "",
        padding: List[float] = []
    ) -> Dict[str, Any]:
        """
        Add a Horizontal Box layout widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            hbox_name: Name to give the new Horizontal Box
            parent_name: Name of parent widget (empty = root)
            size_rule: Slot size rule - "fill" or "" (auto)
            fill_value: Fill coefficient when size_rule is "fill"
            h_align: Horizontal alignment in parent slot
            v_align: Vertical alignment in parent slot
            padding: [L, T, R, B] slot padding
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"blueprint_name": widget_name, "widget_name": hbox_name}
            if parent_name:
                params["parent_name"] = parent_name
            if size_rule:
                params["size_rule"] = size_rule
                params["fill_value"] = fill_value
            if h_align:
                params["h_align"] = h_align
            if v_align:
                params["v_align"] = v_align
            if padding:
                params["padding"] = padding

            response = unreal.send_command("add_horizontal_box_to_widget", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response

        except Exception as e:
            return {"success": False, "message": f"Error adding Horizontal Box: {e}"}

    @mcp.tool()
    def add_border_to_widget(
        ctx: Context,
        widget_name: str,
        border_name: str,
        parent_name: str = "",
        background_color: List[float] = [0.08, 0.08, 0.08, 0.95],
        content_padding: List[float] = [8.0, 8.0, 8.0, 8.0],
        size_rule: str = "",
        fill_value: float = 1.0,
        h_align: str = "",
        v_align: str = "",
        padding: List[float] = [],
        position: List[float] = [0.0, 0.0],
        size: List[float] = [],
        auto_size: bool = False
    ) -> Dict[str, Any]:
        """
        Add a Border widget (background + padding container) to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            border_name: Name to give the new Border
            parent_name: Name of parent widget (empty = root canvas panel)
            background_color: [R, G, B, A] background brush color
            content_padding: [L, T, R, B] inner content padding
            size_rule: Slot size rule for VBox/HBox - "fill" or "" (auto)
            fill_value: Fill coefficient when size_rule is "fill"
            h_align: Horizontal alignment in parent slot
            v_align: Vertical alignment in parent slot
            padding: [L, T, R, B] slot padding
            position: [X, Y] position (Canvas Panel only)
            size: [W, H] size (Canvas Panel only, ignored when auto_size=True)
            auto_size: Auto-size to content (Canvas Panel only)
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {
                "blueprint_name": widget_name,
                "widget_name": border_name,
                "background_color": background_color,
                "content_padding": content_padding,
                "position": position,
                "auto_size": auto_size,
            }
            if size:
                params["size"] = size
            if parent_name:
                params["parent_name"] = parent_name
            if size_rule:
                params["size_rule"] = size_rule
                params["fill_value"] = fill_value
            if h_align:
                params["h_align"] = h_align
            if v_align:
                params["v_align"] = v_align
            if padding:
                params["padding"] = padding

            response = unreal.send_command("add_border_to_widget", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response

        except Exception as e:
            return {"success": False, "message": f"Error adding Border: {e}"}

    @mcp.tool()
    def add_size_box_to_widget(
        ctx: Context,
        widget_name: str,
        size_box_name: str,
        parent_name: str = "",
        width_override: float = 0.0,
        height_override: float = 0.0,
        size_rule: str = "",
        fill_value: float = 1.0,
        h_align: str = "",
        v_align: str = "",
        padding: List[float] = []
    ) -> Dict[str, Any]:
        """
        Add a Size Box widget to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            size_box_name: Name to give the new Size Box
            parent_name: Name of parent widget (empty = root)
            width_override: Fixed width override (0 = disabled)
            height_override: Fixed height override (0 = disabled)
            size_rule: Slot size rule - "fill" or "" (auto)
            fill_value: Fill coefficient when size_rule is "fill"
            h_align: Horizontal alignment in parent slot
            v_align: Vertical alignment in parent slot
            padding: [L, T, R, B] slot padding
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"blueprint_name": widget_name, "widget_name": size_box_name}
            if width_override > 0:
                params["width_override"] = width_override
            if height_override > 0:
                params["height_override"] = height_override
            if parent_name:
                params["parent_name"] = parent_name
            if size_rule:
                params["size_rule"] = size_rule
                params["fill_value"] = fill_value
            if h_align:
                params["h_align"] = h_align
            if v_align:
                params["v_align"] = v_align
            if padding:
                params["padding"] = padding

            response = unreal.send_command("add_size_box_to_widget", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response

        except Exception as e:
            return {"success": False, "message": f"Error adding Size Box: {e}"}

    @mcp.tool()
    def add_overlay_to_widget(
        ctx: Context,
        widget_name: str,
        overlay_name: str,
        parent_name: str = "",
        size_rule: str = "",
        fill_value: float = 1.0,
        h_align: str = "",
        v_align: str = "",
        padding: List[float] = []
    ) -> Dict[str, Any]:
        """
        Add an Overlay widget (z-stack children) to a UMG Widget Blueprint.

        Args:
            widget_name: Name of the target Widget Blueprint
            overlay_name: Name to give the new Overlay
            parent_name: Name of parent widget (empty = root)
            size_rule: Slot size rule - "fill" or "" (auto)
            fill_value: Fill coefficient when size_rule is "fill"
            h_align: Horizontal alignment in parent slot
            v_align: Vertical alignment in parent slot
            padding: [L, T, R, B] slot padding
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "message": "Failed to connect to Unreal Engine"}

            params = {"blueprint_name": widget_name, "widget_name": overlay_name}
            if parent_name:
                params["parent_name"] = parent_name
            if size_rule:
                params["size_rule"] = size_rule
                params["fill_value"] = fill_value
            if h_align:
                params["h_align"] = h_align
            if v_align:
                params["v_align"] = v_align
            if padding:
                params["padding"] = padding

            response = unreal.send_command("add_overlay_to_widget", params)
            if not response:
                return {"success": False, "message": "No response from Unreal Engine"}
            return response

        except Exception as e:
            return {"success": False, "message": f"Error adding Overlay: {e}"}

    logger.info("UMG tools registered successfully")