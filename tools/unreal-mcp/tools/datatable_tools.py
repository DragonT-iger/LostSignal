"""
DataTable Tools for Unreal MCP.

Provides tools for fixing and reimporting DataTable assets.
"""

import logging
from typing import Dict, Any
from mcp.server.fastmcp import FastMCP, Context

logger = logging.getLogger("UnrealMCP")

def register_datatable_tools(mcp: FastMCP):
    """Register DataTable tools with the MCP server."""

    @mcp.tool()
    def fix_datatable_row_struct(
        ctx: Context,
        datatable_path: str,
        row_struct_name: str
    ) -> Dict[str, Any]:
        """Fix a DataTable whose RowStruct is broken (e.g. pointing to a HOTRELOAD transient copy).

        Args:
            datatable_path: Asset path of the DataTable, e.g. /Game/LostSignal/Data/DataTables/DT_CharacterStat
            row_struct_name: Exact C++ struct name (without F prefix), e.g. LSCharacterStatRow

        Returns:
            Dict with success status and fixed asset info
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("fix_datatable_row_struct", {
                "datatable_path": datatable_path,
                "row_struct_name": row_struct_name
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error fixing DataTable row struct: {e}")
            return {"success": False, "error": str(e)}

    @mcp.tool()
    def reimport_datatable_from_csv(
        ctx: Context,
        datatable_path: str,
        csv_path: str,
        row_struct_name: str
    ) -> Dict[str, Any]:
        """Reimport a DataTable from a CSV file, fixing the row struct in the process.

        Args:
            datatable_path: Asset path of the DataTable, e.g. /Game/LostSignal/Data/DataTables/DT_CharacterStat
            csv_path: Absolute filesystem path to the CSV file
            row_struct_name: Exact C++ struct name (without F prefix), e.g. LSCharacterStatRow

        Returns:
            Dict with success status, row count, and any warnings
        """
        from unreal_mcp_server import get_unreal_connection

        try:
            unreal = get_unreal_connection()
            if not unreal:
                return {"success": False, "error": "Failed to connect to Unreal Engine"}

            response = unreal.send_command("reimport_datatable_from_csv", {
                "datatable_path": datatable_path,
                "csv_path": csv_path,
                "row_struct_name": row_struct_name
            })

            if not response:
                return {"success": False, "error": "No response from Unreal Engine"}

            return response

        except Exception as e:
            logger.error(f"Error reimporting DataTable: {e}")
            return {"success": False, "error": str(e)}

    logger.info("DataTable tools registered successfully")
