package main

import (
	"github.com/charmbracelet/bubbles/textinput"
	tea "github.com/charmbracelet/bubbletea"
	"github.com/charmbracelet/lipgloss"
)

type sessionState int

const (
	stateGrid sessionState = iota
	stateInput
	stateSubMenu
)

type model struct {
	state     sessionState
	cursorX   int
	cursorY   int
	width     int
	height    int
	textInput textinput.Model
}

func initialModel() model {
	ti := textinput.New()
	ti.Placeholder = "Type..."
	ti.Prompt = "> "
	
	// Ép style của ô input thành trắng hoàn toàn
	whiteStyle := lipgloss.NewStyle().Foreground(lipgloss.Color("#FFFFFF"))
	ti.PromptStyle = whiteStyle
	ti.TextStyle = whiteStyle
	ti.Cursor.Style = lipgloss.NewStyle().Background(lipgloss.Color("#FFFFFF")).Foreground(lipgloss.Color("#000000")) // Cursor đảo màu
	
	ti.Focus()
	ti.CharLimit = 156
	ti.Width = 30

	return model{
		state:     stateGrid,
		cursorX:   0,
		cursorY:   0,
		textInput: ti,
	}
}

func (m model) Init() tea.Cmd {
	return textinput.Blink
}

func (m model) Update(msg tea.Msg) (tea.Model, tea.Cmd) {
	var cmd tea.Cmd
	var cmds []tea.Cmd

	switch msg := msg.(type) {
	case tea.WindowSizeMsg:
		m.width = msg.Width
		m.height = msg.Height

	case tea.KeyMsg:
		if msg.String() == "ctrl+c" {
			return m, tea.Quit
		}

		switch m.state {
		case stateGrid:
			switch msg.String() {
			case "q":
				return m, tea.Quit
			case "h", "left":
				if m.cursorX > 0 {
					m.cursorX--
				}
			case "l", "right":
				if m.cursorX < 1 {
					m.cursorX++
				}
			case "k", "up":
				if m.cursorY > 0 {
					m.cursorY--
				}
			case "j", "down":
				if m.cursorY < 1 {
					m.cursorY++
				}
			case "enter":
				if m.cursorY == 0 && m.cursorX == 0 {
					m.state = stateInput
					return m, textinput.Blink
				} else if m.cursorY == 0 && m.cursorX == 1 {
					m.state = stateSubMenu
				}
			}

		case stateInput:
			switch msg.String() {
			case "esc":
				m.state = stateGrid
			case "enter":
				m.state = stateGrid
			default:
				m.textInput, cmd = m.textInput.Update(msg)
				cmds = append(cmds, cmd)
			}

		case stateSubMenu:
			switch msg.String() {
			case "q", "esc":
				m.state = stateGrid
			}
		}
	}

	return m, tea.Batch(cmds...)
}

func (m model) View() string {
	if m.width == 0 {
		return "Initializing..."
	}

	// Định nghĩa palette màu Trắng/Xám
	white := lipgloss.Color("#FFFFFF")
	dimGray := lipgloss.Color("240") // Xám tối để tôn phần màu trắng lên

	switch m.state {
	case stateInput:
		// Bọc Text Input trong một hộp viền vuông vức màu trắng
		inputBox := lipgloss.NewStyle().
			Border(lipgloss.NormalBorder()).
			BorderForeground(white).
			Padding(1, 2).
			Render(m.textInput.View())

		ui := lipgloss.JoinVertical(lipgloss.Center,
			lipgloss.NewStyle().Foreground(white).Bold(true).Render("INPUT MODE"),
			"",
			inputBox,
			"",
			lipgloss.NewStyle().Foreground(dimGray).Render("[ESC to return]"),
		)
		return lipgloss.Place(m.width, m.height, lipgloss.Center, lipgloss.Center, ui)

	case stateSubMenu:
		// Bọc Sub Menu trong hộp trắng vuông
		menuBox := lipgloss.NewStyle().
			Border(lipgloss.NormalBorder()).
			BorderForeground(white).
			Padding(2, 4).
			Render("DUMMY SUB MENU\n\n[EMPTY]")

		ui := lipgloss.JoinVertical(lipgloss.Center,
			menuBox,
			"",
			lipgloss.NewStyle().Foreground(dimGray).Render("[q/ESC to return]"),
		)
		return lipgloss.Place(m.width, m.height, lipgloss.Center, lipgloss.Center, ui)
	}

	// === STATE GRID ===
	cellWidth := (m.width / 2) - 2
	cellHeight := (m.height / 2) - 2
	if cellWidth < 0 { cellWidth = 0 }
	if cellHeight < 0 { cellHeight = 0 }

	// Style cơ bản cho các ô không được chọn: Viền mảnh, màu xám chìm, chữ xám
	baseStyle := lipgloss.NewStyle().
		Width(cellWidth).
		Height(cellHeight).
		Align(lipgloss.Center, lipgloss.Center).
		Border(lipgloss.NormalBorder()).
		BorderForeground(dimGray).
		Foreground(dimGray)

	// Style cho ô đang focus: Viền dày (nét đôi hoặc đậm), trắng toát, chữ trắng in đậm
	activeStyle := baseStyle.Copy().
		Border(lipgloss.ThickBorder()).
		BorderForeground(white).
		Foreground(white).
		Bold(true)

	cellContents := []string{
		"TEXT INPUT",
		"SUB MENU",
		"EMPTY",
		"EMPTY",
	}

	var cells []string
	for i := 0; i < 4; i++ {
		cx := i % 2
		cy := i / 2
		style := baseStyle
		if cx == m.cursorX && cy == m.cursorY {
			style = activeStyle
		}
		cells = append(cells, style.Render(cellContents[i]))
	}

	topRow := lipgloss.JoinHorizontal(lipgloss.Top, cells[0], cells[1])
	bottomRow := lipgloss.JoinHorizontal(lipgloss.Top, cells[2], cells[3])
	grid := lipgloss.JoinVertical(lipgloss.Left, topRow, bottomRow)

	return lipgloss.Place(m.width, m.height, lipgloss.Center, lipgloss.Center, grid)
}
