import { Container, Box } from '@chakra-ui/react'
import ControlPanel from './components/ControlPanel'

function App() {
  return (
    <Box minH="100vh" bg="gray.50">
      <Container maxW="container.xl" py={8}>
        <ControlPanel />
      </Container>
    </Box>
  )
}

export default App
