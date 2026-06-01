#!/usr/bin/env python3
"""Tiny helper to print the development index tasks.

This intentionally does not edit the Excel workbook. Use it to keep codebase status visible in CI/logs.
"""
from pathlib import Path

index = Path("docs/development_index.md")
print(index.read_text()[:4000])
