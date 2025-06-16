import { Box, Button, Grid, VStack, Text, Switch, HStack, Textarea } from '@chakra-ui/react';
import { useState, useEffect } from 'react';
import { arduinoService } from '../services/arduinoService';
import { createStandaloneToast } from '@chakra-ui/react';

const { toast } = createStandaloneToast();

const ControlPanel = () => {
  const [isLoading, setIsLoading] = useState<string | null>(null);
  const [stats, setStats] = useState<string[]>([]);
  const [toggleStates, setToggleStates] = useState({
    ultrasonic: false,
    magnetic: false,
    IR: false,
    radiowaves: false,
  });

  // Fetch stats on component mount and every 2 seconds
  useEffect(() => {
    const fetchStats = async () => {
      try {
        const statsData = await arduinoService.getStats();
        // statsData is something like { stats: ["Snorkie", "Wibbo", ...] }
        setStats(statsData.stats || []);
      } catch (error) {
        // Silently fail - display nothing if stats unavailable
        setStats([]);
      }
    };

    fetchStats(); // Initial fetch
    const interval = setInterval(fetchStats, 2000); // Update every 2 seconds

    return () => clearInterval(interval);
  }, []);

  const handleCommand = async (command: () => Promise<any>, buttonName: string) => {
    setIsLoading(buttonName);
    try {
      const response = await command();
      toast({
        title: 'Success',
        description: response.message,
        status: 'success',
        duration: 2000,
        isClosable: true,
      });
    } catch (error) {
      console.log(error);
      toast({
        title: 'Error',
        description: 'Failed to send command to Arduino',
        status: 'error',
        duration: 3000,
        isClosable: true,
      });
    } finally {
      setIsLoading(null);
    }
  };

  const handleToggle = async (toggleName: keyof typeof toggleStates) => {
    const newState = !toggleStates[toggleName];
    setToggleStates(prev => ({ ...prev, [toggleName]: newState }));
    
    try {
      let response;
      switch (toggleName) {
        case 'ultrasonic':
          response = await arduinoService.toggleUltrasonic(newState);
          break;
        case 'magnetic':
          response = await arduinoService.toggleMagnetic(newState);
          break;
        case 'IR':
          response = await arduinoService.toggleIR(newState);
          break;
        case 'radiowaves':
          response = await arduinoService.toggleRadiowaves(newState);
          break;
      }
      
      toast({
        title: 'Success',
        description: response.message || `${toggleName} ${newState ? 'enabled' : 'disabled'}`,
        status: 'success',
        duration: 2000,
        isClosable: true,
      });
    } catch (error) {
      // Revert the toggle state on error
      setToggleStates(prev => ({ ...prev, [toggleName]: !newState }));
      toast({
        title: 'Error',
        description: `Failed to toggle ${toggleName}`,
        status: 'error',
        duration: 3000,
        isClosable: true,
      });
    }
  };

  const buttonStyle = {
    size: 'lg',
    colorScheme: 'blue',
    variant: 'solid',
    _hover: { transform: 'scale(1.05)' },
    transition: 'all 0.2s',
  };

  return (
    <Box p={8} maxW="800px" mx="auto">
      <VStack spacing={8}>
        <Text fontSize="3xl" fontWeight="bold" color="blue.600">
          Arduino Control Panel
        </Text>
        
        {/* Movement Controls */}
        <Grid templateColumns="repeat(3, 1fr)" gap={4} width="100%">
          <Box />
          <Button
            {...buttonStyle}
            isLoading={isLoading === 'up'}
            onClick={() => handleCommand(() => arduinoService.moveUp(), 'up')}
          >
            Up
          </Button>
          <Box />
          
          <Button
            {...buttonStyle}
            isLoading={isLoading === 'left'}
            onClick={() => handleCommand(() => arduinoService.moveLeft(), 'left')}
          >
            Left
          </Button>
          <Button
            {...buttonStyle}
            colorScheme="red"
            isLoading={isLoading === 'stop'}
            onClick={() => handleCommand(() => arduinoService.stop(), 'stop')}
          >
            Stop
          </Button>
          <Button
            {...buttonStyle}
            isLoading={isLoading === 'right'}
            onClick={() => handleCommand(() => arduinoService.moveRight(), 'right')}
          >
            Right
          </Button>
          
          <Box />
          <Button
            {...buttonStyle}
            isLoading={isLoading === 'down'}
          >
            Down
          </Button>
          <Box />
        </Grid>

        {/* Toggle Controls */}
        <VStack spacing={4} width="100%">
          <Text fontSize="xl" fontWeight="semibold">Sensor Controls</Text>
          
          <Grid templateColumns="repeat(2, 1fr)" gap={6} width="100%">
            <HStack justify="space-between">
              <Text>Ultrasonic</Text>
              <Switch
                isChecked={toggleStates.ultrasonic}
                onChange={() => handleToggle('ultrasonic')}
                size="lg"
                colorScheme="blue"
              />
            </HStack>
            
            <HStack justify="space-between">
              <Text>Magnetic</Text>
              <Switch
                isChecked={toggleStates.magnetic}
                onChange={() => handleToggle('magnetic')}
                size="lg"
                colorScheme="purple"
              />
            </HStack>
            
            <HStack justify="space-between">
              <Text>IR</Text>
              <Switch
                isChecked={toggleStates.IR}
                onChange={() => handleToggle('IR')}
                size="lg"
                colorScheme="red"
              />
            </HStack>
            
            <HStack justify="space-between">
              <Text>Radiowaves</Text>
              <Switch
                isChecked={toggleStates.radiowaves}
                onChange={() => handleToggle('radiowaves')}
                size="lg"
                colorScheme="green"
              />
            </HStack>
          </Grid>
        </VStack>

        {/* Stats Display */}
        {stats.length > 0 && (
          <VStack spacing={4} width="100%">
            <Text fontSize="xl" fontWeight="semibold">Arduino Stats</Text>
            <Box
              w="100%"
              p={4}
              bg="gray.100"
              border="1px solid"
              borderColor="gray.300"
              borderRadius="md"
              minH="150px"
              fontFamily="monospace"
              fontSize="sm"
            >
              {stats.map((stat, index) => (
                <Text key={index} mb={1}>
                  {stat}
                </Text>
              ))}
            </Box>
          </VStack>
        )}
      </VStack>
    </Box>
  );
};

export default ControlPanel; 