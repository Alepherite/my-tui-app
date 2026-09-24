package main

import (
	"fmt"
	"os"

	tea "github.com/charmbracelet/bubbletea"
)

func main() {
	// Initialize our TUI model
	m := initialModel()

	// Create a new Bubble Tea program.
	// WithAltScreen() opens the app in a new, full-screen terminal buffer.
	p := tea.NewProgram(m, tea.WithAltScreen())

	if _, err := p.Run(); err != nil {
		fmt.Printf("Error running program: %v", err)
		os.Exit(1)
	}
}
